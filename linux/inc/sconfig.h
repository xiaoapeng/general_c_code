/**
 * @file sconfig.h
 * @brief config 的实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-26
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef _SCONFIG_H_
#define _SCONFIG_H_

#include <memory>
#include <mutex>
#include <list>
#include <string_view>
#include <functional>
#include "toml.hpp"

class Sconfig{
    public:
        using SSconfig = std::shared_ptr<Sconfig>;
        using ConfigChange = std::function<void()>;
        class Author{
            friend class Sconfig;
            public:
                class Write{
                    public:
                        toml::table  &config;
                        Write(Sconfig::Author &author):
                            config(author.ssconfig->toml_config),
                            author(&author){
                                this->author->ssconfig->mtx.lock();
                            }
                        ~Write(){
                            author->ssconfig->mtx.unlock();
                            author->ssconfig->sync();
                            author->ssconfig->config_change(*author);
                        };
                    private:
                        Sconfig::Author *author;
                };
                class Read{
                    public:
                        const toml::table  &config;
                        Read(Sconfig::Author &author):
                            config(author.ssconfig->toml_config),
                            author(&author){
                                this->author->ssconfig->mtx.lock();
                            }
                        ~Read(){
                            author->ssconfig->mtx.unlock();
                        };
                    private:
                        Sconfig::Author *author;
                };
                Author(Sconfig::SSconfig configPtr);
                ~Author();
                void set_configChange_cb(ConfigChange cb);
            private:
                ConfigChange config_change;
                Sconfig::SSconfig ssconfig;
        };


        static SSconfig Create(const std::string_view filename) {
            struct make_shared_enabler : public Sconfig {
                make_shared_enabler(const std::string_view filename): 
                    Sconfig(filename) {}
            };
            return std::make_shared<make_shared_enabler>(filename);
        }

        void parse_file(void);
        void sync(void);
        
    private:
        Sconfig(const std::string_view filename):
            config_file_path(filename){   }
        
        void add_authors(Author &author);
        void remove_authors(Author &author);
        void config_change(Author &author);

        std::list<Author*>              authors;
        std::recursive_mutex            mtx;
        toml::table                     toml_config;
        const std::string               config_file_path;
};




#endif // _SCONFIG_H_

