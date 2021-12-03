## Instructions

本階段為 Client 端程式的開發，同學所撰寫的 Client 端程式必須要能:

+ [x] 向 Server 註冊(註冊時輸入名稱)。(register)
+ [x] 登入助教所提供 Server 端程式(填入使用者名稱、Port Number)。(port number)
+ [ ] 向 Server 要最新的帳戶餘額、上線清單、Publickey 並接收 Server 端的回覆。 (list)
4. 和其它 Client 執行轉帳功能 (不可透過 Server 端轉送)。(transaction)
5. 進行離線動作前需主動告知 Server 端程式。(exit)

## Trouble-shooting

1. `10.211.55.4`
2. on port `8888`

## Steps

1. 先與 server 建立連線 (TCP connection)
2. 接受 server 回傳的第一個訊息 (Connected to the server!)
3. 

```
#define REGISTER 1
#define LOGIN 2
#define LIST 3
#define TRANSACTION 4
#define EXIT 5
```

## Functions

5. Exit

    ```
    *******Session*******
    Bytes written: 20 Bytes read: 32833
    Elapsed time: 42 secs
    Connection closed
    ```