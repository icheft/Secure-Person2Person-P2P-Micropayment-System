# + [ ] Source Code(Client 端程式的原始碼)。
# + [ ] 操作說明文件 PDF 檔(包含如何編譯、執行 Client 端程式，程式執行環境說明，參考資料、來源等)。
# + [ ] Binary 執行檔(已 Compile 及 Linking 完成並可執行的 Client 端程 式)。
# + [ ] 用以編譯程式之 Makefile。請將上述四項檔案壓縮成:
# + 學號_part1.tar.gz (e.g. b027050xx_part1.tar.gz)，

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