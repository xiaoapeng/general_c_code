
#include "sconfig.h"

void Sconfig::parse_file(void){
    std::unique_lock<std::recursive_mutex> auto_lock(mtx);
    toml_config = std::move(toml::parse_file(config_file_path));
}
void Sconfig::sync(void){
    std::unique_lock<std::recursive_mutex> auto_lock(mtx);
    std::ofstream file(config_file_path);
    if (!file.is_open()) 
        throw std::runtime_error("Failed to open file: " + config_file_path);
    file << toml_config;
}

void Sconfig::add_authors(Author &author){
    std::unique_lock<std::recursive_mutex> auto_lock(mtx);
    authors.push_back(&author);
}

void Sconfig::remove_authors(Author &author){
    std::unique_lock<std::recursive_mutex> auto_lock(mtx);
    authors.remove(&author);
}

void Sconfig::config_change(Author &_author){
    std::unique_lock<std::recursive_mutex> auto_lock(mtx);
    for (Author* author : authors) {
        if(author != &_author)
            author->config_change();
    }
}

Sconfig::Author::Author(Sconfig::SSconfig configPtr): ssconfig(configPtr) {
    ssconfig->add_authors(*this);
}

Sconfig::Author::~Author(){
    ssconfig->remove_authors(*this);
}
void Sconfig::Author::set_configChange_cb(ConfigChange cb){
    config_change = cb;
}