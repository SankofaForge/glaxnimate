# TODO

- [ ] **Fix ThorVG's base64 non-ASCII check on unsigned-char platforms.**
  `b64Decode` in `external/thorvg/thorvg/src/common/tvgCompressor.cpp`
  rejects non-ASCII input with `encoded[3] < 0` (lines 67 and 71). `char` is
  unsigned on ARM64, so that comparison is always false: the check never runs,
  and GCC reports `-Wtype-limits`. These are the last two warnings in a clean
  build. Upstream ThorVG has the same code, so fix it there, for example by
  testing `static_cast<unsigned char>(c) >= 0x80`. Then move the submodule,
  which pins a commit from the `glaxpatch` branch of `mbasaglia/thorvg`.
- [ ] **Fix ThorVG's base64 decoder checking the wrong character.** On line 67
  of the same file, the test that decides whether `encoded[2]` ends the input
  checks `encoded[3] < 0` instead of `encoded[2] < 0`. Fix it upstream with the
  item above.
