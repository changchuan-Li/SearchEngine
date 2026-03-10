#include "../include/Pagelib.h"
#include "../include/tinyxml2.h"
#include "../include/DirScanner.h"
#include "../include/Configuration.h"
#include "../include/simhash/Simhasher.hpp"
#include "../include/MysqlHelper.h"

#include <math.h>
#include <fstream>
#include <regex>
#include <iostream>
#include <fstream>
#include <map>

using std::cout;
using std::endl;
using std::ofstream;
using std::map;
using namespace tinyxml2;

PageLib::PageLib(): _config(Configuration::getInstance("../conf/page.conf")), _jieba(_dict.getJieba()){

}

PageLib::~PageLib(){

}

void PageLib::create(){

}

void PageLib::store(){

}

void PageLib::storeToMysql(){

}

void PageLib::handleInvertIndex(){

}

void PageLib::handleWordMap(){

}

void PageLib::handleTopK(){

}

void PageLib::PageDeDuplication(){

}