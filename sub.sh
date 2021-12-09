FILES="docs/phase01-doc.pdf src/ client Makefile"

if [ "$1" = "tar" ]
then
    tar -czf $2_part1.tar.gz $FILES
elif [ "$1" = "zip" ]
then
    zip $2_part1.zip $FILES
else
    echo "please specify zip or tar"
fi