#include <iostream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include "gumbo.h" 
#include <iconv.h>

// Структура для хранения информации о продукте
struct Product {
    std::string name;
    std::string price;
};

// Функция для записи данных, полученных от libcurl, в строку
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Функция для преобразования кодировки из UTF-8 в CP1251 (или другую)
std::string utf8_to_cp1251(const std::string& utf8_str) {
    iconv_t cd = iconv_open("CP1251", "UTF-8");
    if (cd == (iconv_t)-1) {
        std::cerr << "iconv_open failed" << std::endl;
        return "";
    }

    size_t inbytesleft = utf8_str.size();
    size_t outbytesleft = inbytesleft * 2;
    char* inbuf = const_cast<char*>(utf8_str.c_str());
    char* outbuf = new char[outbytesleft];
    char* outbuf_start = outbuf;

    if (iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1) {
        std::cerr << "iconv failed" << std::endl;
        iconv_close(cd);
        delete[] outbuf_start;
        return "";
    }

    std::string result(outbuf_start, outbuf - outbuf_start);
    delete[] outbuf_start;
    iconv_close(cd);
    return result;
}

// Функция для поиска элементов <a> с классом "link link-hover" внутри <div class="list">
void search_for_links_in_list(GumboNode* node, std::vector <Product> & products)
{
    if (node->type != GUMBO_NODE_ELEMENT) return ;

    // Ищем элементы <div> с классом "list"
    //GumboAttribute* div_class;
   /* if (node->v.element.tag == GUMBO_TAG_DIV &&
        (div_class = gumbo_get_attribute(&node->v.element.attributes, "class")) &&
        std::string(div_class->value).find("f_zthV9b- css-0") != std::string::npos) {*/
    
    if (node->v.element.tag == GUMBO_TAG_HEAD) return;

        // Внутри <div class="list"> ищем <a class="link link-hover">
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            GumboNode* child = static_cast<GumboNode*>(children->data[i]);
            if (child->v.element.tag == GUMBO_TAG_HEAD) continue;
            if (child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == GUMBO_TAG_DIV) {
                GumboAttribute* p_attr = gumbo_get_attribute(&child->v.element.attributes, "class");
                Product prod;
                prod.name = " ";
                std::string cp1251_text = " ";
                std::string price;
                if (p_attr && std::string(p_attr->value).find("card-name") != std::string::npos)
                {
                    // Получаем текст внутри элемента <a>
                    if (child->v.element.children.length > 0) 
                    {
                        GumboNode* text_node = static_cast<GumboNode*>(child->v.element.children.data[0]);
                        if (text_node->type == GUMBO_NODE_TEXT) 
                        {
                            std::string utf8_text = text_node->v.text.text;
                            cp1251_text = utf8_to_cp1251(utf8_text);
                            std::cout << cp1251_text << std::endl;
                            prod.name = cp1251_text;
                            products.push_back(prod);
                        }
                    }
                }
                if (p_attr && std::string(p_attr->value).find("basic nw") != std::string::npos)
                {
                    GumboAttribute* price_attr = gumbo_get_attribute(&child->v.element.attributes, "data-price");
                    if (price_attr->name == std::string("data-price")) {
                        std::cout << "data-price: " << price_attr->value << std::endl;
                        products.at(products.size() - 1).price = price_attr->value;
                    }
                }
            }
        }
    //}

    // Рекурсивно обрабатываем дочерние узлы
    // GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        search_for_links_in_list(static_cast<GumboNode*>(children->data[i]), products);
    }
}



int main() {
    setlocale(LC_ALL, "russian");
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    // Инициализация curl
    curl = curl_easy_init();
    if (curl) {
        // URL страницы
        curl_easy_setopt(curl, CURLOPT_URL, "https://dostavka.dixy.ru/catalog/chipsy-orekhi-i-sneki/chipsy/");

        // Функция обратного вызова для сохранения данных
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        //curl_easy_setopt(curl, CURLOPT_PROXY, CURLPROXY_HTTPS);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3");
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        


        // Выполнение запроса
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        // Проверка успешности запроса
        if (res == CURLE_OK) {
            // Парсинг HTML и извлечение продуктов
            GumboOutput* output = gumbo_parse(readBuffer.c_str());
            std::vector<Product> products;
            search_for_links_in_list(output->root, products);

            // Вывод информации о продуктах
            for (const auto& product : products) {
                std::cout << "Product: " << product.name << ", Price: " << product.price << std::endl;
            }
            std::cout << products.size() << std::endl;
        }
        else {
            std::cerr << "Error: " << curl_easy_strerror(res) << std::endl;
        }
    }
    return 0;
}


