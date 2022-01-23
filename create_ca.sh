CERT_PATH=certs

if [ "$1" = "cert" ]
then
    openssl req -new -x509 -config $CERT_PATH/$2_ssc.conf -keyout $CERT_PATH/$3.key -out $CERT_PATH/$3.crt
    openssl x509 -pubkey -noout -in $CERT_PATH/$3.crt  > $CERT_PATH/$3.pem
elif [ "$1" = "CA" ]
then
    rm $CERT_PATH/CA.pem
    for i in $CERT_PATH/*.crt ; do
        openssl x509 -in $i -text >> $CERT_PATH/CA.pem
    done
else
    echo "Choose cert or CA"
fi