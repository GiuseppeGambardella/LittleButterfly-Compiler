#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>

// Memref 1D descriptor: memref<?xi8>
// LLVM lo passa come 5 argomenti: (allocated, aligned, offset, size0, stride0)
struct MemRef1D {
    char* allocated;
    char* aligned;
    int64_t offset;
    int64_t size;
    int64_t stride;
};

static inline char* dataPtr(const MemRef1D& m) {
    // per memref di char: base = aligned + offset * stride
    return m.aligned + (m.offset * m.stride);
}

static inline MemRef1D makeMemRefFromOwnedCString(char* heapPtr) {
    // size/stride sono “informativi” per noi; per printf ci basta il terminatore '\0'
    MemRef1D m;
    m.allocated = heapPtr;
    m.aligned   = heapPtr;
    m.offset    = 0;
    m.size      = (int64_t)std::strlen(heapPtr) + 1; // includo '\0'
    m.stride    = 1;
    return m;
}

extern "C" {

    // =========================
    // PRINT (scalari)
    // =========================
    void print_int(int32_t n) {
        std::printf("%d\n", n);
        std::fflush(stdout);
    }

    int32_t random(int32_t max) {
        if (max <= 0) return 0;
        static bool seeded = false;
        if (!seeded) {
            std::srand(static_cast<unsigned int>(std::time(nullptr)));
            seeded = true;
        }

        return std::rand() % (max + 1);
    }

    void print_double(double d) {
        std::printf("%f\n", d);
        std::fflush(stdout);
    }

    void print_char(int8_t c) {
        std::printf("%c\n", (char)c);
        std::fflush(stdout);
    }

    // =========================
    // PRINT STRING (memref ABI)
    // Firma: void print_string(ptr, ptr, i64, i64, i64)
    // =========================
    void print_string(char* allocated, char* aligned, int64_t offset, int64_t size, int64_t stride) {
        (void)allocated; (void)size; (void)stride; // non necessari per printf
        if (!aligned) {
            std::printf("(null)\n");
            std::fflush(stdout);
            return;
        }
        char* s = aligned + offset * stride;
        std::printf("%s\n", s);
        std::fflush(stdout);
    }

    // =========================
    // READ
    // =========================
    int32_t read_int() {
        int32_t x;
        std::scanf("%d", &x);
        return x;
    }

    double read_double() {
        double x;
        std::scanf("%lf", &x);
        return x;
    }

    int8_t read_char() {
        char x;
        std::scanf(" %c", &x);
        return (int8_t)x;
    }

    MemRef1D read_string() {
        // Legge una linea intera (fino a newline)
        char buffer[1024];

        if (!std::fgets(buffer, sizeof(buffer), stdin)) {
            // EOF o errore → stringa vuota
            char* heap = _strdup("");
            return makeMemRefFromOwnedCString(heap);
        }

        // Rimuove newline finale se presente
        size_t len = std::strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        char* heap = _strdup(buffer);
        return makeMemRefFromOwnedCString(heap);
    }


    // =========================
    // TO_STRING: ritornano MemRef1D (struct)
    // IR: {ptr, ptr, i64, [1 x i64], [1 x i64]}
    // In pratica torna un descriptor 1D
    // =========================

    MemRef1D to_string_int(int32_t x) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d", x);
        char* heap = _strdup(buf);              // su Windows va bene _strdup
        return makeMemRefFromOwnedCString(heap);
    }

    MemRef1D to_string_double(double x) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%f", x);
        char* heap = _strdup(buf);
        return makeMemRefFromOwnedCString(heap);
    }

    MemRef1D to_string_bool(bool b) {
        char* heap = _strdup(b ? "true" : "false");
        return makeMemRefFromOwnedCString(heap);
    }

    MemRef1D to_string_char(int8_t c) {
        char buf[2];
        buf[0] = (char)c;
        buf[1] = '\0';
        char* heap = _strdup(buf);
        return makeMemRefFromOwnedCString(heap);
    }

    // Firma IR: to_string_string(ptr, ptr, i64, i64, i64) -> memref
    // cioè prende una stringa memref e la duplica (ownership heap)
    MemRef1D to_string_string(char* allocated, char* aligned, int64_t offset, int64_t size, int64_t stride) {
        (void)allocated; (void)size; (void)stride;
        char* s = aligned ? (aligned + offset * stride) : (char*)"";
        char* heap = _strdup(s);
        return makeMemRefFromOwnedCString(heap);
    }

    // =========================
    // CONCAT: prende due memref string e torna memref string
    // Firma IR: concat_strings(5 args for a, 5 args for b) -> MemRef1D
    // =========================
    MemRef1D concat_strings(
        char* a_alloc, char* a_align, int64_t a_off, int64_t a_size, int64_t a_stride,
        char* b_alloc, char* b_align, int64_t b_off, int64_t b_size, int64_t b_stride
    ) {
        (void)a_size; (void)b_size;

        char* a = a_align ? (a_align + a_off * a_stride) : (char*)"";
        char* b = b_align ? (b_align + b_off * b_stride) : (char*)"";

        size_t la = std::strlen(a);
        size_t lb = std::strlen(b);

        char* r = (char*)std::malloc(la + lb + 1);
        std::memcpy(r, a, la);
        std::memcpy(r + la, b, lb + 1);

        // ownership: libera le stringhe “owned”
        // Nota: per le literal globali, MLIR spesso passa allocated=poison (0xDEADBEEF)
        // quindi NON dobbiamo free() su quelle. Noi liberiamo solo se allocated sembra “valida”.
        // Regola pratica: liberi SOLO se allocated == aligned e non è poison.
        auto isPoison = [](char* p) {
            return (uintptr_t)p == (uintptr_t)0xDEADBEEF;
        };

        if (a_alloc && !isPoison(a_alloc) && a_alloc == a_align) std::free(a_alloc);
        if (b_alloc && !isPoison(b_alloc) && b_alloc == b_align) std::free(b_alloc);

        return makeMemRefFromOwnedCString(r);
    }

    int8_t strcmp_strings(
        char* a_alloc, char* a_align, int64_t a_off, int64_t a_size, int64_t a_stride,
        char* b_alloc, char* b_align, int64_t b_off, int64_t b_size, int64_t b_stride
    ) {
        (void)a_alloc; (void)a_size; (void)b_alloc; (void)b_size;

        char* a = a_align ? (a_align + a_off * a_stride) : (char*)"";
        char* b = b_align ? (b_align + b_off * b_stride) : (char*)"";

        int cmp = std::strcmp(a, b);
        if (cmp < 0) return -1;
        if (cmp > 0) return 1;
        return 0;
    }
}
