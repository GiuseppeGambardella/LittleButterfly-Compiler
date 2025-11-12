# 🦋 LittleButterfly Compiler

A compiler for a custom, C-like language, built for the **Programming Language Engineering** university course.

---

## 🚀 About the Project

The **LittleButterfly Compiler** is a university project that demonstrates the principles of compiler construction. It implements a compiler for a custom-invented, C-like language.

The language follows the general syntax, structure, and semantics of C. However, it introduces a unique linguistic twist: a transformation rule applied **exclusively to C keywords and standard library function names.**

## 🔎 The "Butterfly" Language Rule

The core feature of the LittleButterfly language is a "vowel expansion" transformation. Every vowel (`a`, `e`, `i`, `o`, `u`) found in a C keyword or standard library function is expanded by adding an 'f' and repeating the vowel.

The formal production rule for this change is:
`vowel ::= vowelfvowel`

### The Crucial Distinction

This transformation **only** applies to the language's built-in vocabulary.
* **✅ Transformed:** C keywords (`int`, `while`, `if`), standard library functions (`printf`, `main`, `malloc`).
* **❌ Not Transformed:** User-defined identifiers (variable names, custom function names) and string literals.

This design choice creates a parsing challenge where the lexer and parser must distinguish between the "Butterfly" vocabulary and standard user input.

### Examples of Transformation

| Standard C | LittleButterfly Language |
| :--- | :--- |
| `int` | **`ifint`** |
| `char` | **`chafar`** |
| `while` | **`whifilefe`** |
| `if` | **`ifif`** |
| `return` | **`refetufurn`** |
| `main` | **`mafaifin`** |
| `printf` | **`prifintf`** |

---

## 💻 Code Comparison

Here is a simple program in standard C, followed by its equivalent in the LittleButterfly language.

### Standard C
```c
int main() {
    int counter = 10;
    while (counter > 0) {
        printf("Count: %d\n", counter);
        counter--;
    }
    return 0;
}
```

```c
// Note how user-defined names 'counter' and the string literal are unchanged.

ifint mafaifin() {
    ifint counter = 10; // 'ifint' is transformed, 'counter' is not

    whifilefe (counter > 0) { // 'whifilefe' is transformed
        prifintf("Count: %d\n", counter); // 'prifintf' is transformed, string is not
        counter--;
    }
    
    refetufurn 0; // 'refetufurn' is transformed
}
```
