# CMSC 124 Problem Set 1 Analysis

## Pair

- Drew T. Cudiamat (`@pojomo2`)
- Samantha F. Mok (`@smnthmk`)

## Analysis:

### Question 1

**Pick 3 of the 10 categories. For each, pick a language that gives it to you for free and say what that language pays for it. "Python has dictionaries" isn't an answer. What does Python's dictionary cost in memory or in speed compared to what you built, and where would you notice?**

#### Map — Python (`dict`)

&nbsp;&nbsp;&nbsp;&nbsp;Python's dictionaries provide convenient syntax literals (`{key: value}`) that automate key-value mapping, hashing, and collision handling for the developer ([Python Dict C-API](https://docs.python.org/3/c-api/dict.html)). However, instead of operating on raw values directly, every key and value is wrapped in a small object that stores extra information, such as a type pointer and a reference count ([Python Object Protocol](https://docs.python.org/3/c-api/object.html)). These wrapped objects are stored alongside an internal hash table and an index array, which makes Python dictionaries use more memory and take more time per lookup than low-level C code that works with raw data in plain arrays and structs.

In `dt_map`, keys are raw text with no wrapper. Python keys are objects with extra fields. Our values sit directly inside the entry, while Python values are pointers to objects stored elsewhere. Our lookup calls `strcmp` on raw bytes, without having to take extra steps to hash and compare wrapped objects.

#### List — Kotlin (`List` / `MutableList`)

&nbsp;&nbsp;&nbsp;&nbsp;In Kotlin, lists can be defined as `List` or `MutableList`, giving developers compile-time safety against accidental changes ([Kotlin Collections Overview](https://kotlinlang.org/docs/collections-overview.html)). A `List` cannot be modified through that reference, while a `MutableList` allows adding, replacing, and removing elements. Built-in operations like `.filter()` and `.windowed()` make data transformations safe and readable by handling index ranges and resizing automatically, without manual pointer math. On the other hand, chaining these operations creates a new list at every step, which adds work for the garbage collector unless the chain is wrapped in a lazy `Sequence` ([Kotlin Sequences](https://kotlinlang.org/docs/sequences.html)). Additionally, a generic list like `List<Int>` forces every number into a wrapper object, which uses more memory than a plain integer. Kotlin provides special array types like `IntArray` to store raw numbers when that matters ([Kotlin Primitive Arrays](https://kotlinlang.org/docs/arrays.html)).

&nbsp;&nbsp;&nbsp;&nbsp;In `dt_list`, every element is stored directly inside its cell, with no wrapper object. Our list has no built-in filter or map operations. Kotlin one-liners become multi-line loops in C, but they control the exact number of allocations. `dt_list_free` frees one cell and leaves the tail alone, because the driver's registry owns every cell. Kotlin's garbage collector handles sharing, at the cost of tracing and pauses.

#### String — Java (`java.lang.String`)

&nbsp;&nbsp;&nbsp;&nbsp;Strings in Java offer high developer convenience through immutability safety, automatic memory allocation, and garbage collection that eliminates manual memory management ([Oracle Java String API](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/lang/String.html)). They also come with standard text tools like `.split()` and `.trim()`, along with runtime bounds checks that safely throw exceptions instead of causing memory corruption. As for the trade-offs, strings cannot be changed in place, so modifications require a separate object like `StringBuilder` ([Java StringBuilder API](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/lang/StringBuilder.html)). Sensitive string data also cannot be cleared from memory on demand; it stays in RAM until the garbage collector reclaims it. Additionally, the internal storage of a string uses either one byte or two bytes per character depending on the text, and every character access must check which format is in use ([JEP 254 Compact Strings](https://openjdk.org/jeps/254)). That check adds work compared to a raw C pointer.

&nbsp;&nbsp;&nbsp;&nbsp;In `dt_str`, our string is a pointer, a length, and a capacity, unlike a Java `String` which is an object that points to a byte array. Also, the string has no format flag to check on every access. The cost Java pays for automatic bounds checking and compact storage is at C's price of requiring every caller to check the returned status and free the string manually.

## Question 2

**You wrote the tag check in `dt_value_as_int` by hand. Some languages don't let you. They make the tagged union a language construct, so the compiler writes the check for you, refuses to compile a read that skips it, and refuses to compile a set of cases that misses one. Rust's `enum` and `match` work this way, and so do ML's datatypes and Swift's enumerations with associated values. What does the C version let you do that a compiler enforcing the check wouldn't, and is any of it worth wanting?**

&nbsp;&nbsp;&nbsp;&nbsp;In C, the tag and payload are stored separately, so the compiler does not guarantee that they match. That lets the program choose and manipulate the representation directly—for example, it can set or inspect the tag and payload independently. It also means the compiler cannot prevent code from reading the wrong union member or alert the programmer when a case has been forgotten. In this project, dt_value_as_int handles that responsibility by checking for DT_INT before reading the integer payload.

&nbsp;&nbsp;&nbsp;&nbsp;A language such as Rust makes the tagged union a language construct: an enum groups each tag with its corresponding payload, and match requires the code to handle every case. That gives up some direct control over the representation in exchange for compiler-enforced checks. C’s flexibility can be useful when a specific layout or low-level control is important, but it comes with the risk of mismatched tags, invalid accesses, and missed cases. For this project, that trade-off does not seem worth much: the manual check in C works, but having the compiler enforce the tag-payload relationship would make mistakes harder to introduce.

&nbsp;&nbsp;&nbsp;&nbsp;The tagged union is defined in dt.h, and the manual tag check is implemented in dt_value.c.


## Question 3

**Your `dt_map` keeps insertion order separately from the hash buckets, which is memory spent on something no lookup uses. Argue the other side: describe a design that drops it, say what breaks, and say whether you'd ship it.**

&nbsp;&nbsp;&nbsp;&nbsp;Without a separate insertion-order array, the only way to walk the map is bucket by bucket, following each chain to the end. That order comes from the hash, so two keys added one after the other can print in any order the hash feels like. Updating a key will not move it, but removing and reinserting one would still shuffle things around. The expected-output files can't be written, because the same commands might print differently on different runs.

&nbsp;&nbsp;&nbsp;&nbsp;So, the thing that breaks is printing, which is why the order array exists. We wouldn't ship it, since we'd just be saving a little memory by giving up what the map is supposed to do for the printer.

## Question 4

**Compare access after release with an allocation that remains unreleased at the driver's final check. What damage can each cause in a long-running server? How does that answer change for a command-line tool that exits in a second?**

&nbsp;&nbsp;&nbsp;&nbsp;Accessing memory after it has been released can cause unpredictable behavior. The program might crash or corrupt data if that memory has already been reused. An allocation left unreleased wastes memory; in a long-running server, repeated leaks can build up until the server slows down or runs out of memory. A command-line program that exits after a second is less likely to suffer lasting harm from a leak because the operating system reclaims its memory on exit. But accessing released memory can still cause problems before the program exits.
