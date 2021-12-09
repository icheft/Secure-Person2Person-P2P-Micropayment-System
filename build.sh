IN_DIR="docs/"
OUT_DIR="docs/"
DOC_PREFIX="phase01"

pandoc "$1" -o "$OUT_DIR$DOC_PREFIX.pdf" --from markdown --template eisvogel --listings --pdf-engine "xelatex" --toc