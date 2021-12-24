IN_DIR="docs/"
OUT_DIR="docs/"
DOC_PREFIX="phase02"

pandoc "$1" -o "$OUT_DIR$2.pdf" --from markdown --template eisvogel --listings --pdf-engine "xelatex" --toc