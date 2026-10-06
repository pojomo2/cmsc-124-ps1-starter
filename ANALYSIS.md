1. Pick 3 of the 10 categories. For each, pick a language that gives it to you for free and say what that language pays for it. "Python has dictionaries" isn't an answer. What does Python's dictionary cost in memory or in speed compared to what you built, and where would you notice?

2. You wrote the tag check in dt_value_as_int by hand. Some languages don't let you. They make the tagged union a language construct, so the compiler writes the check for you, refuses to compile a read that skips it, and refuses to compile a set of cases that misses one. Rust's enum and match work this way, and so do ML's datatypes and Swift's enumerations with associated values. What does the C version let you do that a compiler enforcing the check wouldn't, and is any of it worth wanting?

Answer: In C, the tag and payload are stored separately, so the compiler does not guarantee that they match. That lets the program choose and manipulate the representation directly—for example, it can set or inspect the tag and payload independently. It also means the compiler cannot prevent code from reading the wrong union member or alert the programmer when a case has been forgotten. In this project, dt_value_as_int handles that responsibility by checking for DT_INT before reading the integer payload.

A language such as Rust makes the tagged union a language construct: an enum groups each tag with its corresponding payload, and match requires the code to handle every case. That gives up some direct control over the representation in exchange for compiler-enforced checks. C’s flexibility can be useful when a specific layout or low-level control is important, but it comes with the risk of mismatched tags, invalid accesses, and missed cases. For this project, that trade-off does not seem worth much: the manual check in C works, but having the compiler enforce the tag-payload relationship would make mistakes harder to introduce.

The tagged union is defined in dt.h, and the manual tag check is implemented in dt_value.c.


3. Your dt_map keeps insertion order separately from the hash buckets, which is memory spent on something no lookup uses. Argue the other side: describe a design that drops it, say what breaks, and say whether you'd ship it.

4. Compare access after release with an allocation that remains unreleased at the driver's final check. What damage can each cause in a long-running server? How does that answer change for a command-line tool that exits in a second?

Answer: Accessing memory after it has been released can cause unpredictable behavior. The program might crash or corrupt data if that memory has already been reused. An allocation left unreleased wastes memory; in a long-running server, repeated leaks can build up until the server slows down or runs out of memory. A command-line program that exits after a second is less likely to suffer lasting harm from a leak because the operating system reclaims its memory on exit. But accessing released memory can still cause problems before the program exits.
