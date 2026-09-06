#pragma once

// RU/EN для всего чита. Флаг живёт в Vars::Menu::bRussian (сохраняется в конфиг),
// T() переводит подписи меню по EN-оригиналу. Внутренние ID везде остаются
// английскими: помощь, анимации и конфиг от языка не зависят.
// Файл в UTF-8, проект компилируется с /utf-8, DrawManager конвертит из CP_UTF8.

#include "../Vars.h"

#include <map>
#include <string>

namespace Lang
{
	inline bool IsRu() { return Vars::Menu::bRussian; }

	inline const char* T(const char* szEn)
	{
		if (!IsRu() || !szEn || !szEn[0])
			return szEn;
		static const std::map<std::string, std::string> tbl = {
			{"Visuals","Визуал"}, {"Move","Движение"}, {"View","Вид"}, {"Combat","Бой"}, {"Misc","Разное"},
			{"ESP box","ESP бокс"}, {"ESP health bar","ESP полоса HP"}, {"ESP name","ESP ники"},
			{"ESP distance","ESP дистанция"}, {"ESP items","ESP предметы"}, {"ESP commons","ESP обычные"},
			{"ESP special infected","ESP особые"}, {"ESP witch","ESP ведьма"},
			{"Snaplines","Снаплайны"}, {"Filled boxes","Заливка боксов"},
			{"HP text near bar","HP текст"}, {"Weapon text","Текст оружия"},
			{"Chams","Чамсы"}, {"Chams through walls","Чамсы сквозь стены"}, {"Chams palette >","Палитра чамсов >"},
			{"No visual recoil","Без виз. отдачи"},
			{"Bunny hop","Бхоп"}, {"Bhop: Perfect >","Бхоп: Идеал >"}, {"Bhop: Legit >","Бхоп: Легит >"},
			{"Auto strafe","Авто-стрейф"},
			{"Strafe: Legit >","Стрейф: Легит >"}, {"Strafe: Rage >","Стрейф: Рейдж >"},
			{"Strafe: W-Only >","Стрейф: W-Only >"}, {"Strafe: Directional >","Стрейф: Направл. >"},
			{"Bhop delay","Задержка бхопа"}, {"Edge jump","Эдж-джамп"}, {"Edge bug","Эджбаг"}, {"Jump bug","Джампбаг"},
			{"Null movement","Нулл-мувмент"}, {"Fast stop","Быстрый стоп"}, {"Speed HUD","Скорость HUD"},
			{"Jump stats","Стата прыжка"}, {"Prestrafe","Престрейф"}, {"Long jump helper","Лонгджамп"},
			{"Auto duck","Авто-присед"},
			{"FOV x100 (view+model)","FOV x100 (мир+модель)"}, {"No fog","Без тумана"},
			{"Third person","3-е лицо"}, {"3rd person distance","Дистанция камеры"},
			{"Crosshair","Прицел"}, {"Crosshair size","Размер прицела"}, {"FPS / pos overlay","FPS / поз. оверлей"},
			{"Aimbot","Аимбот"}, {"Auto shoot","Авто-огонь"}, {"Silent aim","Сайлент-аим"},
			{"Hitbox: Head >","Хитбокс: Голова >"}, {"Hitbox: Center >","Хитбокс: Тело >"},
			{"Priority: FOV >","Приоритет: FOV >"}, {"Priority: Distance >","Приоритет: Дистанция >"},
			{"Visible only","Только видимые"}, {"Skip incapped","Без лежачих"},
			{"Target commons","Таргет: обычные"}, {"Target specials","Таргет: особые"},
			{"Aim FOV x10","FOV аима x10"}, {"Smoothing","Сглаживание"},
			{"Aimbot key","Клавиша аима"}, {"Trigger bot","Триггербот"},
			{"Trigger visible only","Триггер видим. only"}, {"Trigger key","Клавиша триггера"},
			{"Auto pistol","Авто-пистолет"}, {"Auto shove","Авто-толчок"},
			{"No spread","Без разброса"}, {"Killfeed","Киллфид"},
			{"Radar","Радар"}, {"Spectators","Наблюдатели"},
			{"Spectators (%d)","Наблюдатели (%d)"},
			{"Alerts","Алерты"}, {"Tank alert","Алерт танка"}, {"Witch alert","Алерт ведьмы"},
			{"TANK","ТАНК"}, {"WITCH","ВЕДЬМА"},
			{"Save config","Сохранить конфиг"}, {"Load config","Загрузить конфиг"},
			{"Menu key","Клавиша меню"}, {"STYLE","СТИЛЬ"},
			{"Menu accent","Акцент меню"}, {"ESP enemy","ESP враги"}, {"ESP ally","ESP союзники"},
			{"Language: English >","Язык: Английский >"}, {"Language: Russian >","Язык: Русский >"},
			{"F11 = unload cheat","F11 = выгрузить чит"},
			{"drag header | WASD free | F11 unload | %d fps","тащи за шапку | WASD свободны | F11 выгрузка | %d fps"},
			{"[press key]","[нажми клавишу]"}, {"off","выкл"},
			{"killed","убил"}, {"died","умер"}, {"strafes","стрейфы"}, {"sync","синхр."},
		};
		auto it = tbl.find(szEn);
		return it != tbl.end() ? it->second.c_str() : szEn;
	}
}
