//Подключение библиотек
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <json/json.h>
#include <chrono>
#include <thread>

//Версия VK API
#define API_VER "5.131"

//Имя файла с конфигом скрипта
std::string configName;

//Переменная для хранения полученного JSON'а
std::string responseData;

//Функция для записи ответа в переменную выше
static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    ((std::string*)userdata)->append(ptr, size * nmemb);
    return size * nmemb;
}

int main() {
    //Стартовое сообщение, запрос имени конфига
    std::cout << "VKSimpleAntiKick by Jefter, v1.0" << std::endl << std::endl;
    std::cout << "Input config file name: ";
    std::cin >> configName;

    //Объекты для работы парсера ("база данных" и буфер)
    Json::Value root;
    Json::Reader reader;

    //Парсим конфиг
    std::ifstream file;
    file.open(configName);
    if (!file) std::cout << "Can't open config with this name. Aborting..." << std::endl;
    else file >> responseData;
    bool parsingSuccessful = reader.parse(responseData, root);

    if (!parsingSuccessful) {
        std::cout << "Parsing error. Aborting..." << std::endl;
        return 1;
    }
    else std::cout << "Config loaded succesfully." << std::endl;

    std::string ORIG_TOKEN = root["orig_token"].asString();
    std::string TWINK_TOKEN = root["twink_token"].asString();
    std::string OCHAT_ID = root["orig_chat_id"].asString();
    std::string TCHAT_ID = root["twink_chat_id"].asString();
    std::string ORIG_ID;
    std::string TWINK_ID;
    
    //Инициализация libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    //Создание объекта libcurl
    CURL* curl = curl_easy_init();
    
    //Настройка параметров объекта, линк с функцией записи ответа
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

    CURLcode res;

    long http_code = 0;
    
    //URL для GET запросов
    //Проверка пользователя на кик из чата (1)
    std::string checkUserURL = std::string("https://api.vk.com/method/messages.getHistory")
    						+ "?access_token=" + ORIG_TOKEN
    						+ "&count=1"
    						+ "&user_id=" + OCHAT_ID
    						+ "&v=" + API_VER;

    //Получение ID основного аккаунта (2)
    std::string getOrigID = std::string("https://api.vk.com/method/account.getProfileInfo")
                                        + "?access_token=" + ORIG_TOKEN
                                        + "&v=" + API_VER;

    //Получение ID твинка (3)
    std::string getTwinkID = std::string("https://api.vk.com/method/account.getProfileInfo")
                                        + "?access_token=" + TWINK_TOKEN
                                        + "&v=" + API_VER;

    responseData = "";

    //Отправка запроса 2
    curl_easy_setopt(curl, CURLOPT_URL, getOrigID.c_str());
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl << std::endl;
    }
    else {
        http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }
    
    //Парсинг ответа и запись ID в переменную
    parsingSuccessful = reader.parse(responseData, root);
    if (!parsingSuccessful) return 1;
    else ORIG_ID = root["response"]["id"].asString();

    responseData = "";

    //Отправка запроса 3
    curl_easy_setopt(curl, CURLOPT_URL, getTwinkID.c_str());
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl << std::endl;
    }
    else {
        http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }

    //Парсинг ответа и запись ID в переменную
    parsingSuccessful = reader.parse(responseData, root);
    if (!parsingSuccessful) return 1;
    else TWINK_ID = root["response"]["id"].asString();

    //URL для GET запроса
    //Возврат основного аккаунта в чат (4)
    std::string saveUserURL = std::string("https://api.vk.com/method/messages.addChatUser")
    			    	            	+ "?access_token=" + TWINK_TOKEN
    			                  		+ "&chat_id=" + TCHAT_ID
    		                    		+ "&user_id=" + ORIG_ID
    				            		+ "&v=" + API_VER;

    //Выход твинка из чата (5)
    std::string leaveChatURL = std::string("https://api.vk.com/method/messages.removeChatUser")
                                        + "?access_token=" + TWINK_TOKEN
                                        + "&chat_id=" + TCHAT_ID
                                        + "&user_id=483255030"
                                        + "&v=" + API_VER;

    //Вход твинка в чат (6)
    std::string logChatURL = std::string("https://api.vk.com/method/messages.addChatUser")
                                        + "?access_token=" + TWINK_TOKEN
    			                  		+ "&chat_id=" + TCHAT_ID
    		                    		+ "&user_id=" + TWINK_ID
    				            		+ "&v=" + API_VER;

    //Уведомление о начале работы скрипта
    std::cout << "Script started, waiting for event..." << std::endl;
    
    while (true) {
        responseData = "";
        //Отправка запроса 1
        curl_easy_setopt(curl, CURLOPT_URL, checkUserURL.c_str());
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl << std::endl;
        }
        else {
            http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        }
        
        //Парсим полученый JSON
        bool parsingSuccessful = reader.parse(responseData, root);

        responseData = "";
        
        if (!parsingSuccessful) {
            std::cerr << "Failed to parse JSON string\n";
            return 1;
        }
        
        //Получаем необходимые значения для проверки
        Json::Value& userCheck = root["response"]["items"][0]["action"];

        //Проверяем последнее сообщение на кик пользователя с ID основного аккаунта
        if (userCheck["type"].asString() == "chat_kick_user") {
            if (userCheck["member_id"].asString() == ORIG_ID) {

                //Отправка запроса 5
                curl_easy_setopt(curl, CURLOPT_URL, logChatURL.c_str());
                res = curl_easy_perform(curl);

                //Отправка запроса 4
                curl_easy_setopt(curl, CURLOPT_URL, saveUserURL.c_str());
                res = curl_easy_perform(curl);

                if (res != CURLE_OK) {
                    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl << std::endl;
                }
                else {
                    long http_code = 0;
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                    std::cout << "Kick attempt, user saved." << std::endl << "Waiting next event..." << std::endl;
                }

                //Отправка запроса 6
                curl_easy_setopt(curl, CURLOPT_URL, leaveChatURL.c_str());
                res = curl_easy_perform(curl);
            }
        }

        //Задержка выполнения скрипта
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    //Деинициализация libcurl в случае, если true == false (что)
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    
    return 0;
}
