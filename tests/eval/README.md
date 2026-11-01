# Reading eval

Checks how real models read a handwritten page: corrections written under an earlier answer, follow-up questions, new questions and names. Each case in `cases.json` is a page (`q` lines in a handwriting-like font, `a` lines in Caveat like the tablet's answers), a word the answer must `mention`, and optionally a word it must `avoid`.

```sh
OPENROUTER_API_KEY=... tests/eval/run.sh             # each case once, free model and Haiku
OPENROUTER_API_KEY=... tests/eval/run.sh 3 anthropic/claude-haiku-5.5
```

It sends real requests: Haiku costs about $0.0007 a page, and free models count against OpenRouter's daily limit. The pages are typeset, not handwritten, so the eval tests how the prompt handles a page, not how well a model reads messy handwriting.
