#pragma once
#include "uf.h"
#include <string>
#include <vector>
#include "FeatureAtributesManager.h"
struct type_electrode;//Тип электрода
struct standart_weld;//Стандарт шва
struct design_weld;//Условное обозначение
struct order_num_weld;//Порядковый номер шва
//Сборная структура данных для отображения в таблице
struct order_num_weld;
struct type_electrode
{
	std::string type_electrode_str;
	std::vector<standart_weld> w_standart_list;
};
struct standart_weld {
	std::string standart_weld_str;
	std::vector<design_weld> design_weld_list;
};
struct design_weld {
	std::string design_weld_str;
	std::vector<order_num_weld> order_num_list;
};
struct order_num_weld {
	order_num_weld() : length(0.0), size_weld(1), mass(0.0) {}
	std::string order_num_weld_str;
	double length;
	int size_weld;
	double mass;
};
struct composed_data {
	std::string type_electrode_str;
	std::string standart_weld_str;
	std::string design_weld_str;
	order_num_weld num_weld;
	FeatureAtributesManager* weld_mngr;
	bool operator ==(const composed_data &rhs) const {
		return this->type_electrode_str == rhs.type_electrode_str && this->standart_weld_str == rhs.standart_weld_str 
			&& this->design_weld_str == rhs.design_weld_str && this->num_weld.order_num_weld_str == rhs.num_weld.order_num_weld_str;
	}
	bool operator !=(const composed_data &rhs) const {
		return !((*this) == rhs);
	}
	bool operator < (const composed_data &rhs) const {
		int th = atoi(this->num_weld.order_num_weld_str.c_str());
		return th < atoi(rhs.num_weld.order_num_weld_str.c_str());
	}
};
///Получить данные для заполнения таблицы
typedef std::vector<type_electrode> table_data;
table_data ask_data_table();