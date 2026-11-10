# `transform` property parser reads past the end of the value (heap-buffer-overflow)

`Rml::PropertyParserTransform::Scan` (`Source/Core/PropertyParserTransform.cpp:170`) compares the start of the
remaining value against each function keyword with `memcmp(str, keyword, strlen(keyword))` without first checking
that `strlen(keyword)` bytes are left. When a `transform` value holds one complete function followed by a
truncated keyword, the compare reads beyond the end of the heap-allocated value string.

Reproduce (needs an ASan build, as built by `mayhem/build.sh`):

    /mayhem/fuzz_style_sheet-standalone mayhem/fuzz_style_sheet/known-findings/transform-memcmp-overread/repro.rcss

Expected: `AddressSanitizer: heap-buffer-overflow ... READ of size 11` in `memcmp` called from
`PropertyParserTransform::Scan`, reached from `StyleSheetParser::ReadProperties`.

Not in `testsuite/`: a seed that crashes would stop every run that replays the corpus.
