PART=3
FILES="docs/phase01-doc.pdf docs/phase02-doc.pdf docs/phase03-doc.pdf src/ include/ certs/ client server Makefile create_ca.sh"

if [ "$1" = "tar" ]
then
    tar -czf $2_part$PART.tar.gz $FILES
elif [ "$1" = "zip" ]
then
    zip $2_part$PART.zip $FILES
else
    echo "please specify zip or tar"
fi