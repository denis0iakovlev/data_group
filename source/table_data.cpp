#include "table_data.h"
#include "uf_ui.h"
#include "uf_part.h"
#include "uf_assem.h"
#include "uf_attr.h"
#include "uf_undo.h"
#include <NXOpen/NXException.hxx>
#include <sstream>
#include <algorithm>
#include "rough_symbol_manager.h"
#include "nx_kitsolution.sdk/headers/nx_sdk_nx_helper.h"
//Various weld attribute for using  in table
//attributes titles for grouping on type electrode
#define TYPE_ELECTRODE "TYPE_ELECTRODE"
#define DESIGN_ELECTRODE "DESIGNATION_ELECTRODE"
#define STANDART_ELECTRODE "STANDART_ELECTRODE"
//
#define W_STANDRT "W_STANDART"
#define DESIGNATION_SIGN_WELD "DESIGNATION_SIGN_WELD"
#define DESIGNATION_NUM_WELD "DESIGNATION_NUM_WELD"
#define WELD_LEG "WELD_LEG"
#define WG_NUM "WG_NUM"
//Signs Attributes
#define WELD_REIN_REMOVE "REINFORCEMENT_REMOVE"
#define REIN_REMOVE_SIGNS  "<%REIN_REM> "
#define WELD_FINISH_SMOOTH_TRANS "FINISH_SMOOTH_TRANS"
#define FINISH_SMOOTH_TRANS_SIGNS "<%FIN_SMTH> "
//For roughness
#define WELD_CONTROL_REQ "WELD_CONTROL_REQ"
//Types for loading
#define TYPE_WELD_PART "Pm8_WeldContain"
//
#define MASS "WELD_MASS"
#define SEGMENT_LENGTH "Segment Length"
#define LENGTH "Length"
//
#define RUSSIAN_YES "Да"
//
#define ERROR_EMPTY_COLLECTION "В рабочей детали отстутствуют швы"
typedef std::map<std::string, std::vector<FeatureAtributesManager*>> group_attr_mngr;
typedef bool(*pred)(composed_data lhs, composed_data rhs);
//Групировать данные 
std::vector<std::vector<composed_data>> group_data(std::vector<composed_data> list, pred compartor);
//Компараторы
order_num_weld create_num_weld(std::vector<composed_data> data);
//Функция для группировки по типу электрода или марки проволоки
std::string get_type_electrode(FeatureAtributesManager* sttr_mngr);
//Групировка по условному обозначению
std::string get_conditional_designation(FeatureAtributesManager* sttr_mngr);
//Групировка по порядковому номеру
std::string order_num_weld_predicate(FeatureAtributesManager* sttr_mngr);
//Получить занчение аттрибута , если атрибут отстутствет вернуть пустую строку
std::string get_attr_value_or_empty(FeatureAtributesManager* sttr_mngr, const std::string title);
//Получить значение аттрибута , если он отстутсвует то вернуть 0 
double get_real_attribute_value(tag_t object, char * title);
//
bool is_equal_type_electrode(composed_data lhs, composed_data rhs);
//
bool is_equal_standart_weld(composed_data lhs, composed_data rhs);

bool is_design_weld(composed_data lhs, composed_data rhs);

table_data ask_data_table()
{
	tag_t disp_part = UF_PART_ask_display_part();
	tag_t root_part = UF_ASSEM_ask_root_part_occ(disp_part);
	tag_p_t children(0);
	int val_children = UF_ASSEM_ask_part_occ_children(root_part, &children);
	if (val_children == 0) {
		throw std::exception("Draw assembly is wrong. dont occurences in first level");
	}
	std::vector<std::string> loading_type(1, TYPE_WELD_PART);
	nx_sdk_nx_helper::utilities_assembly::load_occurences_on_type(children[0], loading_type);
	auto weld_manager_collection = FeatureAttributeManager_collection::get_feature_collection(children[0])->get_feature_attr_collection();
	if (weld_manager_collection.size() == 0)
	{
		throw std::exception(ERROR_EMPTY_COLLECTION);
	}
	//Заполнить данные для каждого шва
	std::vector<composed_data> linear_data;
	for (auto &m : weld_manager_collection) {
		composed_data adding_data;
		adding_data.weld_mngr = m;
		adding_data.type_electrode_str = get_type_electrode(m);
		adding_data.standart_weld_str = get_attr_value_or_empty(m, W_STANDRT);
		adding_data.design_weld_str = get_conditional_designation(m);
		adding_data.num_weld.order_num_weld_str = get_attr_value_or_empty(m, WG_NUM);
		linear_data.push_back(adding_data);
	}
	//Отсортировать по WG_NUM
	std::sort(linear_data.begin(), linear_data.end());
	//Сгрупировать швы
	std::vector<std::vector<composed_data>> group_type_electrode = group_data(linear_data, is_equal_type_electrode);
	table_data res;
	for (auto &v : group_type_electrode) {
		type_electrode tp;
		tp.type_electrode_str = v[0].type_electrode_str;
		//Групировать стандарту 
		auto weld_standart_group = group_data(v, is_equal_standart_weld);
		for (auto v1 : weld_standart_group) {
			standart_weld std;
			std.standart_weld_str = v1[0].standart_weld_str;
			//Групировать по условному обозначению
			auto  сonditinal_group = group_data(v1, is_design_weld);
			for (auto v2 : сonditinal_group) {
				design_weld des;
				des.design_weld_str = v2[0].design_weld_str;
				auto num_weld_group = group_data(v2, [](composed_data lhs, composed_data rhs) { return lhs.num_weld.order_num_weld_str == rhs.num_weld.order_num_weld_str; });
				for (auto v3 : num_weld_group) {
					des.order_num_list.push_back(create_num_weld(v3));
				}
				std.design_weld_list.push_back(des);
			}
			tp.w_standart_list.push_back(std);
		}
		res.push_back(tp);
	}

	return res;
}


std::vector<std::vector<composed_data>> group_data(std::vector<composed_data> list, pred comparator)
{
	std::vector<std::vector<composed_data>> res;
	auto beg = list.begin();
	auto end = list.begin() + 1;
	std::vector<composed_data> add_list;
	while (end != list.end()) {
		if (!comparator(*beg, *end)) {
			add_list = std::vector<composed_data>(beg, end);
			res.push_back(add_list);
			beg = end;
		}
		end++;
	}
	add_list = std::vector<composed_data>(beg, end);
	res.push_back(add_list);
	return res;
}


order_num_weld create_num_weld(std::vector<composed_data> data)
{
	order_num_weld res;
	if (data.size() == 0)
		return res;
	std::stringstream buff;
	if (data.size() > 1) {
		buff << data.size() << order_num_weld_predicate(data[0].weld_mngr);
	}
	else {
		buff << order_num_weld_predicate(data[0].weld_mngr);
	}
	res.order_num_weld_str = buff.str();
	res.size_weld = data.size();
	for (auto &v : data) {
		res.mass += get_real_attribute_value(v.weld_mngr->GetWeldFeature().feature_attr, MASS);
		//Длина может быть под одним из двух аттрибутов
		double length = get_real_attribute_value(v.weld_mngr->GetWeldFeature().feature_attr, SEGMENT_LENGTH);
		if (length == 0.0) {
			length = get_real_attribute_value(v.weld_mngr->GetWeldFeature().feature_attr, LENGTH);
		}
		res.length += length;
	}
	return res;
}

std::string get_type_electrode(FeatureAtributesManager * attr_mngr)
{
	std::vector<std::string> type_electrode_attr_titles(3);
	type_electrode_attr_titles[0] = TYPE_ELECTRODE;
	type_electrode_attr_titles[1] = DESIGN_ELECTRODE;
	type_electrode_attr_titles[2] = STANDART_ELECTRODE;
	std::stringstream buff;
	for (auto &v : type_electrode_attr_titles) {
		auto val = get_attr_value_or_empty(attr_mngr, v);
		if (val.size() > 0 && val != " ")
			buff << val << " ";
	}

	return buff.str();
}

std::string get_conditional_designation(FeatureAtributesManager * sttr_mngr)
{
	std::string res;
	res = get_attr_value_or_empty(sttr_mngr, DESIGNATION_SIGN_WELD) + get_attr_value_or_empty(sttr_mngr, DESIGNATION_NUM_WELD);
	auto katet = get_attr_value_or_empty(sttr_mngr, WELD_LEG);
	if (katet.size() > 0) {
		katet = "-<%KATET>" + katet;
	}
	res += katet;
	//Получить
	auto control_req = get_attr_value_or_empty(sttr_mngr, WELD_REIN_REMOVE);
	std::string sign;
	if (control_req == RUSSIAN_YES) {
		sign += REIN_REMOVE_SIGNS;
	}
	auto finish_trans = get_attr_value_or_empty(sttr_mngr, WELD_FINISH_SMOOTH_TRANS);
	if (finish_trans == RUSSIAN_YES) {
		sign += FINISH_SMOOTH_TRANS_SIGNS;
	}
	auto rough_value = get_attr_value_or_empty(sttr_mngr, WELD_CONTROL_REQ);
	if (rough_value.size() > 0 && rough_value != " ") {
		try {
			auto symbol_mngr = rough_symbol_mananger::get_rough_symbol_manager();
			tag_t symbol = symbol_mngr->get_rough_symbol_tag(rough_value);
			UF_OBJ_set_blank_status(symbol, UF_OBJ_BLANKED);
			char* handler(0);
			UF_TAG_ask_handle_from_tag(symbol, &handler);
			std::stringstream buff;
			buff << "<C0.65><!SYM=" << handler << "><C>";
			sign += buff.str();
			UF_free(handler);
		}
		catch (NXOpen::NXException &e) {
			UF_UI_open_listing_window();
			UF_UI_write_listing_window(e.Message());
		}
		catch (std::exception &e) {
			UF_UI_open_listing_window();
			UF_UI_write_listing_window(e.what());
		}
		catch (...) {
			UF_UI_open_listing_window();
			UF_UI_write_listing_window("Is occured undefined error");
		}
	}
	res += sign;
	return res;
}

std::string order_num_weld_predicate(FeatureAtributesManager * sttr_mngr)
{
	std::string res = get_attr_value_or_empty(sttr_mngr, WG_NUM);
	if (res.size() > 0) {
		res = "№" + res;
	}
	return res;
}

std::string get_attr_value_or_empty(FeatureAtributesManager * sttr_mngr, const std::string title)
{
	std::string res;
	try {
		res = sttr_mngr->GetValueOnTitle(title);
	}
	catch (std::exception &e) {
		UF_print_syslog((char*)e.what(), 0);
	}
	catch (...) {
		std::stringstream buff;
		buff << "Не удалось получить атрибут " << title << " у обьекта " << sttr_mngr->GetWeldFeature().feature_builder << std::endl;
		UF_print_syslog((char*)buff.str().c_str(), 0);
	}
	return res;
}

double get_real_attribute_value(tag_t object, char * title)
{
	double res = 0.0;
	UF_ATTR_iterator_t iter;
	UF_ATTR_info_t info;
	UF_ATTR_init_user_attribute_iterator(&iter);
	iter.title = title;
	iter.type = UF_ATTR_real;
	logical is_has;
	UF_ATTR_get_user_attribute(object, &iter, &info, &is_has);
	if (!is_has)
		return res;
	return info.real_value;
}

bool is_equal_type_electrode(composed_data lhs, composed_data rhs)
{
	return (lhs.type_electrode_str == rhs.type_electrode_str);
}

bool is_equal_standart_weld(composed_data lhs, composed_data rhs)
{
	return (lhs.standart_weld_str == rhs.standart_weld_str);
}

bool is_design_weld(composed_data lhs, composed_data rhs)
{
	return (lhs.design_weld_str == rhs.design_weld_str);
}
