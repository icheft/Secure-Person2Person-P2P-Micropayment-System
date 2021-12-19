PART=2
FILES="docs/phase01-doc.pdf docs/phase02-doc.pdf src/ include/ client server Makefile"

if [ "$1" = "tar" ]
then
    tar -czf $2_part$(PART).tar.gz $FILES
elif [ "$1" = "zip" ]
then
    zip $2_part$(PART).zip $FILES
else
    echo "please specify zip or tar"
fi