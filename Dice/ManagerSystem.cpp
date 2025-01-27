 /*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123溯洄
 * Copyright (C) 2019-2024 String.Empty
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <string_view>
#include "filesystem.hpp"
#include "ManagerSystem.h"

#include "CharacterCard.h"
#include "GlobalVar.h"
#include "DDAPI.h"
#include "CQTools.h"
#include "DiceSession.h"
#include "DiceSelfData.h"
#include "DiceCensor.h"
#include "DiceMod.h"
#include "BlackListManager.h"

std::filesystem::path dirExe;
std::filesystem::path DiceDir("DiceData");

const dict<short> mChatConf{
	//0-群管理员，2-信任2级，3-信任3级，4-管理员，5-系统操作
	{"忽略", 4},
	{"停用指令", 0},
	{"禁用回复", 0},
	{"禁用jrrp", 0},
	{"禁用draw", 0},
	{"禁用me", 0},
	{"禁用help", 0},
	{"禁用ob", 0},
	{"许可使用", 1},
	{"未审核", 1},
	{"免清", 2},
	{"免黑", 4},
	{"协议无效", 3},
};

User& getUser(long long uid){
	if (TinyList.count(uid))uid = TinyList[uid];
	if (!UserList.count(uid))UserList.emplace(uid,std::make_shared<User>(uid));
	return *UserList[uid];
}
AttrVar getUserItem(long long uid, const string& item) {
	if (!uid)return {};
	if (TinyList.count(uid))uid = TinyList[uid];
	if (item == "name") {
		if (string name{ DD::getQQNick(uid) }; !name.empty())
			return name;
	}
	if (item == "nick") {
		return getName(uid);
	}
	else if (item == "trust") {
		return trustedQQ(uid);
	}
	else if (item == "danger") {
		return blacklist->get_user_danger(uid);
	}
	else if (item == "isDiceMaid") {
		return DD::isDiceMaid(uid);
	}
	else if (item.find("nick#") == 0) {
		long long gid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		return getName(uid, gid);
	}
	else if (UserList.count(uid)) {
		User& user{ getUser(uid) };
		if (item == "firstCreate") return (long long)user.tCreated;
		else  if (item == "lastUpdate") return (long long)user.updated();
		else if (item == "nn") {
			if (string nick; user.getNick(nick))return nick;
		}
		else if (item.find("nn#") == 0) {
			string nick;
			long long gid{ 0 };
			if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
				gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
			}
			if (user.getNick(nick, gid))return nick;
		}
		else if (user.is(item)) {
			return user.get(item);
		}
	}
	if (item == "gender") {
		string ret;
		if (string data{ fifo_json{
			{ "action","getGender" },
			{ "sid",console.DiceMaid },
			{ "uid",uid },
		}.dump() }; DD::getExtra(data, ret)) {
			return fifo_json::parse(ret, nullptr, false);
		}
	}
	return {};
}
AttrVar getGroupItem(long long id, const string& item) {
	if (!id)return {};
	if (item == "size") {
		return (int)DD::getGroupSize(id).currSize;
	}
	else if (item == "maxsize") {
		return (int)DD::getGroupSize(id).maxSize;
	}
	else if (item.find("card#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		if (string card{ DD::getGroupNick(id, uid) }; !card.empty())return card;
	}
	else if (item.find("auth#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		if (int auth{ DD::getGroupAuth(id, uid, 0) }; auth)return auth;
	}
	else if (item.find("lst#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		return DD::getGroupLastMsg(id, uid);
	}
	else if (ChatList.count(id)) {
		Chat& grp{ chat(id) };
		if (item == "name") {
			if (string name{ DD::getGroupName(id) }; !name.empty()) {
				grp.name(name);
				return name;
			}
		}
		else if (item == "firstCreate") {
			return (long long)grp.tCreated;
		}
		else if (item == "lastUpdate") {
			return (long long)grp.updated();
		}
		else if (grp.has(item)) {
			return grp.get(item);
		}
	}
	else if (item == "name") {
		if (string name{ DD::getGroupName(id) }; !name.empty())return name;
	}
	return {};
}
AttrVar getSelfItem(string item) {
	AttrVar var;
	if (item.empty())return var;
	if (item[0] == '&')item = fmt->format(item);
	if (console.intDefault.count(item))return console[item];
	if ((var = getUserItem(console.DiceMaid, item)).is_null()) {
		string file, sub;
		while (!(sub = splitOnce(item)).empty()) {
			file += sub;
			if (selfdata_byStem.count(file)
				&& !(var = selfdata_byStem[file]->data.to_obj()->index(item)).is_null()) {
				return var;
			}
			file += ".";
		}
	}
	return var;
}
AttrVar getContextItem(const AttrObject& context, string item, bool isTrust) {
	if (!context)return {};
	if (item.empty())return context;
	AttrVar var;
	if (item[0] == '&')item = fmt->format(item, context);
	string sub;
	if (!context->empty()) {
		if (context->has(item))return context->get(item);
		if (MsgIndexs.count(item))return MsgIndexs[item](context);
		if (item.find(':') <= item.find('.'))return var;
		if (!(sub = splitOnce(item)).empty()) {
			if (auto sub_tab{ getContextItem(context,sub,isTrust) }; sub_tab.is_table()) {
				return getContextItem(sub_tab.to_obj(), item);
			}
			if (sub == "user")return getUserItem(context->get_ll("uid"), item);
			if (isTrust && (sub == "grp" || sub == "group"))return getGroupItem(context->get_ll("gid"), item);
		}
	}
	if (isTrust) {
		if (sub.empty())sub = splitOnce(item);
		if (sub == "self") {
			return item.empty() ? AttrVar(getMsg("strSelfCall")) : getSelfItem(item);
		}
		else if (selfdata_byStem.count(sub)) {
			var = selfdata_byStem[sub]->data.to_obj()->index(item);
		}
	}
	return var;
}

[[nodiscard]] bool User::empty() const {
	return (!nTrust) && (!updated()) && dict.empty() && strNick.empty();
}
void User::setConf(const string& key, const AttrVar& val)
{
	if (key.empty())return;
	std::lock_guard<std::mutex> lock_queue(ex_user);
	if (val)set(key, val);
	else reset(key);
}
void User::rmConf(const string& key)
{
	if (key.empty())return;
	std::lock_guard<std::mutex> lock_queue(ex_user);
	reset(key);
}

bool User::has(const string& key)const {
	static std::unordered_set<string> items{ "name", "nick", "nn", "trust", "danger", "isDiceMaid" };
	return (dict.count(key) && !dict.at(key).is_null())
		|| items.count(key);
}
AttrVar User::get(const string& item, const AttrVar& val)const {
	if (!item.empty()) {
		if (auto it{ dict.find(item) }; it != dict.end()) {
			return it->second;
		}
		if (item == "name") {
			if (string name{ DD::getQQNick(ID) }; !name.empty())
				return name;
		}
		else if (item == "nick") {
			return getName(ID);
		}
		else if (item == "trust") {
			return ID == console ? 256
				: ID == console.DiceMaid ? 255
				: nTrust;
		}
		else if (item == "danger") {
			return blacklist->get_user_danger(ID);
		}
		else if (item.find("nick#") == 0) {
			long long gid{ 0 };
			if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
				gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
			}
			return getName(ID, gid);
		}
		else if (item == "nn") {
			if (string nick; getNick(nick))return nick;
		}
		else if (item.find("nn#") == 0) {
			long long gid{ 0 };
			if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
				gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
			}
			if (string nick; getNick(nick, gid))return nick;
		}
		else if (item == "isDiceMaid") {
			return DD::isDiceMaid(ID);
		}
	}
	return val;
}
User& User::trust(int n) {
	dict["trust"] = nTrust = n;
	return *this;
}
bool User::getNick(string& nick, long long group) const
{
	if (auto it = strNick.find(group); it != strNick.end() 
		|| (it = strNick.find(0)) != strNick.end())
	{
		nick = it->second;
		return true;
	}
	else if (strNick.size() == 1) {
		nick = strNick.begin()->second;
	}
	return false;
}
void User::writeb(std::ofstream& fout) const
{
	std::lock_guard<std::mutex> lock_queue(ex_user);
	if (!empty()) {
		fwrite(fout, string("Cfg"));
		AnysTable::writeb(fout);
	}
	if (!strNick.empty()) {
		fwrite(fout, string("NN"));
		fwrite(fout, strNick);
	}
	fwrite(fout, string("END"));
}
void User::old_readb(std::ifstream& fin)
{
	std::lock_guard<std::mutex> lock_queue(ex_user);
	fread<long long>(fin);
	map<string, int> intConf;
	fread(fin, intConf);
	for (auto& [key, val] : intConf) {
		set(key, val);
	}
	map<string, string> strConf;
	fread(fin, strConf);
	for (auto& [key, val] : strConf) {
		set(key, val);
	}
	fread(fin, strNick);
}
void User::readb(std::ifstream& fin)
{
	std::lock_guard<std::mutex> lock_queue(ex_user);
	string tag;
	while ((tag = fread<string>(fin)) != "END") {
		if (tag == "Cfg")AnysTable::readb(fin);
		else if (tag == "Conf")AnysTable::readgb(fin);
		else if (tag == "NN")fread(fin, strNick);
		else if (tag == "Nick") {
			if (short len = fread<short>(fin); len > 0) while (len--) {
				long long key = fread<long long>(fin);
				strNick[key] = GBKtoUTF8(fread<string>(fin));
			}
		}
		else if (tag == "ID")fread<long long>(fin); //ignored
	}
	nTrust = get_int("trust");
	if (has("tCreated"))tCreated = get_ll("tCreated");
	if (has("tinyID"))TinyList.emplace(get_ll("tinyID"), ID);
}
int trustedQQ(long long uid){
	if (TinyList.count(uid))uid = TinyList[uid];
	if (uid == console)return 256;
	if (uid == console.DiceMaid)return 255;
	else if (!UserList.count(uid))return 0;
	else return UserList[uid]->nTrust;
}

int clearUser() {
	vector<long long> UserDelete;
	time_t tNow{ time(nullptr) };
	time_t userline{ tNow - console["InactiveUserLine"] * (time_t)86400 };
	bool isClearInactive{ console["InactiveUserLine"] > 0 };
	for (const auto& [uid, user] : UserList) {
		if (user->nTrust > 0 || console.is_self(uid))continue;
		if (user->empty()) {
			UserDelete.push_back(uid);
		}
		else if (isClearInactive) {
			time_t tLast{ user->updated() };
			if (!tLast)tLast = user->tCreated;
			if (auto s{ sessions.get_if({ uid }) })
				tLast = s->tUpdate > tLast ? s->tUpdate : tLast;
			if (tLast >= userline)continue;
			UserDelete.push_back(uid);
			if (PList.count(uid)) {
				PList.erase(uid);
			}
		}
	}
	for (auto uid : UserDelete) {
		UserList.erase(uid);
		if (sessions.has_session(uid))sessions.over(uid);
	}
	return UserDelete.size();
}
int clearGroup() {
	if (console["InactiveGroupLine"] <= 0)return 0;
	vector<long long> GrpDelete;
	time_t tNow{ time(nullptr) };
	time_t grpline{ tNow - console["InactiveGroupLine"] * (time_t)86400 };
	for (const auto& [id, grp] : ChatList) {
		if (grp->is_except() || grp->is("免清") || grp->is("忽略"))continue;
		time_t tLast{ grp->updated() };
		if (auto s{ sessions.get_if({ 0,id }) })
			tLast = s->tUpdate > tLast ? s->tUpdate : tLast;
		if (tLast && tLast < grpline)GrpDelete.push_back(id);
	}
	for (auto id : GrpDelete) {
		ChatList.erase(id);
		if (sessions.has_session({ 0,id }))sessions.over({ 0,id });
	}
	return GrpDelete.size();
}

unordered_map<long long, unordered_map<long long, std::pair<time_t, string>>>skipCards;
string getName(long long uid, long long GroupID){
	if (!uid)return {};
	// Self
	if (uid == console.DiceMaid) return getMsg("strSelfName");

	// nn
	string nick;
	if (UserList.count(uid) && getUser(uid).getNick(nick, GroupID)) return nick;

	// GroupCard
	if (GroupID){
		if (auto& card{ skipCards[GroupID][uid] };
				time(nullptr) < card.first){
			nick = card.second;
		}
		else {
			vector<string>kw;
			nick = strip(msg_decode(nick = DD::getGroupNick(GroupID, uid)));
			if (censor.search(nick, kw) > trustedQQ(uid)) {
				nick.clear();
			}
			card = { time(nullptr) + 600 ,nick };
		}
		if (!nick.empty()) return nick;
	}

	// QQNick
	if (auto& card{ skipCards[0][uid] };
		time(nullptr) < card.first) {
		nick = card.second;
	}
	else {
		vector<string>kw;
		nick = strip(msg_decode(nick = DD::getQQNick(uid)));
		if (censor.search(nick, kw) > trustedQQ(uid)) {
			nick = getMsg("stranger") + "(" + to_string(uid) + ")";
		}
		card = { time(nullptr) + 600 ,nick };
	}
	if (!nick.empty()) return nick;

	// Unknown
	return (UserList.count(uid) ? getMsg("strCallUser") : getMsg("stranger")) + "(" + to_string(uid) + ")";
}
AttrVar idx_nick(const AttrObject& eve) {
	if (eve->has("nick"))return eve->get("nick");
	if (!eve->has("uid"))return {};
	long long uid{ eve->get_ll("uid") };
	long long gid{ eve->get_ll("gid") };
	return eve->at("nick") = getName(uid, gid);
}

string filter_CQcode(const string& raw, long long fromGID){
	string msg{ raw };
	size_t posL(0), posR(0);
	while ((posL = msg.find(CQ_AT)) != string::npos)
	{
		//检查at格式
		if ((posR = msg.find(']', posL)) != string::npos)
		{
			std::string_view stvQQ(msg);
			stvQQ = stvQQ.substr(posL + 10, posR - posL - 10);
			//检查QQ号格式
			bool isDig = true;
			for (auto ch: stvQQ) 
			{
				if (!isdigit(static_cast<unsigned char>(ch)))
				{
					isDig = false;
					break;
				}
			}
			//转义
			if (isDig && posR - posL < 29) 
			{
				msg.replace(posL, posR - posL + 1, "@" + getName(stoll(string(stvQQ)), fromGID));
			}
			else if (stvQQ == "all") 
			{
				msg.replace(posL, posR - posL + 1, "@全体成员");
			}
			else
			{
				msg.replace(posL + 1, 9, "@");
			}
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_IMAGE)) != string::npos) {
		//检查at格式
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, posR - posL - 1, "图片");
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_FACE)) != string::npos) {
		//检查at格式
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, posR - posL - 1, "表情");
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_POKE)) != string::npos) {
		//检查at格式
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, 11, "戳一戳");
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_FILE)) != string::npos) {
		//检查at格式
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, posR - posL - 1, "文件");
		}
		else return msg;
	}
	while ((posL = msg.find("[CQ:")) != string::npos)
	{
		if (size_t posR = msg.find(']', posL); posR != string::npos) 
		{
			msg.erase(posL, posR - posL + 1);
		}
		else return msg;
	}
	return msg;
}
string forward_filter(const string& raw, long long fromGID) {
	string msg{ raw };
	size_t posL(0);
	while ((posL = msg.find(CQ_AT)) != string::npos)
	{
		//检查at格式
		if (size_t posR = msg.find(']', posL); posR != string::npos)
		{
			std::string_view stvQQ(msg);
			stvQQ = stvQQ.substr(posL + 10, posR - posL - 10);
			//检查QQ号格式
			bool isDig = true;
			for (auto ch : stvQQ)
			{
				if (!isdigit(static_cast<unsigned char>(ch)))
				{
					isDig = false;
					break;
				}
			}
			//转义
			if (isDig && posR - posL < 29)
			{
				msg.replace(posL, posR - posL + 1, "@" + getName(stoll(string(stvQQ)), fromGID));
			}
			else if (stvQQ == "all")
			{
				msg.replace(posL, posR - posL + 1, "@全体成员");
			}
			else
			{
				msg.replace(posL + 1, 9, "@");
			}
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_POKE)) != string::npos) {
		//检查at格式
		if (size_t posR = msg.find(']', posL); posR != string::npos) {
			msg.replace(posL + 1, 11, "戳一戳");
		}
		else return msg;
	}
	return msg;
}

Chat& chat(long long id)
{
	if (!ChatList.count(id)) {
		ChatList.emplace(id, std::make_shared<Chat>(id));
	}
	return *ChatList[id];
}

bool Chat::has(const string& key)const {
	static std::unordered_set<string> items{ "name", "size", "maxsize", "firstCreate", "lastUpdate" };
	return (dict.count(key) && !dict.at(key).is_null())
		|| items.count(key);
}
AttrVar Chat::get(const string& item, const AttrVar& val)const {
	if (!item.empty()) {
		if (auto it{ dict.find(item) }; it != dict.end()) {
			return it->second;
		}
		if (item == "name") {
			if (string name{ DD::getGroupName(ID) }; !name.empty())return Name = name;
		}
		else if (item == "size") {
			return (int)DD::getGroupSize(ID).currSize;
		}
		else if (item == "maxsize") {
			return (int)DD::getGroupSize(ID).maxSize;
		}
		else if (item == "firstCreate") {
			return (long long)tCreated;
		}
		else if (item == "lastUpdate") {
			return (long long)updated();
		}
		else if (item.find("card#") == 0) {
			long long uid{ 0 };
			if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
				uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
			}
			if (string card{ DD::getGroupNick(ID, uid) }; !card.empty())return card;
		}
		else if (item.find("auth#") == 0) {
			long long uid{ 0 };
			if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
				uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
			}
			if (int auth{ DD::getGroupAuth(ID, uid, 0) }; auth)return auth;
		}
		else if (item.find("lst#") == 0) {
			long long uid{ 0 };
			if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
				uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
			}
			return DD::getGroupLastMsg(ID, uid);
		}
	}
	return val;
}
Chat& Chat::name(const string& s){
	if (!s.empty())at("name") = Name = s;
	return *this;
}
void Chat::leave(const string& msg) {
	if (!msg.empty()) {
		DD::sendGroupMsg(ID, msg);
		std::this_thread::sleep_for(500ms);
	}
	DD::setGroupLeave(ID);
	reset("已入群").rmLst();
}
bool Chat::is_except()const {
	return has("免黑") || has("协议无效");
}
void Chat::writeb(std::ofstream& fout) const{
	//dict["inviter"] = (long long)inviter;
	fwrite(fout, ID);
	if (!Name.empty())
	{
		fwrite(fout, static_cast<short>(4));
		fwrite(fout, Name);
	}
	if (!empty())
	{
		fwrite(fout, static_cast<short>(11));
		AnysTable::writeb(fout);
	}
	if (!ChConf.empty())
	{
		fwrite(fout, static_cast<short>(21));
		fwrite(fout, ChConf);
	}
	fwrite(fout, static_cast<short>(-1));
}
void Chat::readb(std::ifstream& fin)
{
	fread<long long>(fin);
	short tag = fread<short>(fin);
	while (tag != -1)
	{
		switch (tag)
		{
		case 0:
			Name = GBKtoUTF8(fread<string>(fin));
			break;
		case 4:
			Name = fread<string>(fin);
			break;
		case 1:
			for (auto& key : fread<string, true>(fin)) {
				set(key, true);
			}
			break;
		case 2:
			for (auto& [key, val] : fread<string, int>(fin)) {
				set(key, val);
			}
			break;
		case 3:
			for (auto& [key, val] : fread<string, string>(fin)) {
				set(key, val);
			}
			break;
		case 11:
			AnysTable::readb(fin);
			break;
		case 10:
			AnysTable::readgb(fin);
			break;
		case 21:
			fread(fin, ChConf);
			break;
		case 20:
			if (short len = fread<short>(fin); len > 0) while (len--) {
				long long key = fread<long long>(fin);
				ChConf[key].readgb(fin);
			}
			break;
		default:
			return;
		}
		tag = fread<short>(fin);
	}
	if (has("Name"))Name = get_str("Name");
	inviter = get_ll("inviter");
	if (has("tCreated"))tCreated = get_ll("tCreated");
}
int groupset(long long id, const string& st){
	return ChatList.count(id) ? ChatList[id]->is(st) : -1;
}

Chat& Chat::update() {
	dict["tUpdated"] = (long long)time(nullptr);
	return *this;
}
Chat& Chat::update(time_t tt) {
	dict["tUpdated"] = (long long)tt;
	return *this;
}
Chat& Chat::setLst(time_t t) {
	dict["lastMsg"] = (long long)t;
	return *this;
}
string Chat::print()const{
	if (string name{ DD::getGroupName(ID) }; !name.empty())Name = name;
	if (!Name.empty())return "[" + Name + "](" + to_string(ID) + ")";
	return "群(" + to_string(ID) + ")";
}
void Chat::invited(long long id) {
	set("inviter", inviter = id);
}

#ifdef _WIN32

DWORD getRamPort()
{
	MEMORYSTATUSEX memory_status;
	memory_status.dwLength = sizeof(memory_status);
	GlobalMemoryStatusEx(&memory_status);
	return memory_status.dwMemoryLoad;
}

__int64 compareFileTime(const FILETIME& ft1, const FILETIME& ft2)
{
	__int64 t1 = ft1.dwHighDateTime;
	t1 = t1 << 32 | ft1.dwLowDateTime;
	__int64 t2 = ft2.dwHighDateTime;
	t2 = t2 << 32 | ft2.dwLowDateTime;
	return t1 - t2;
}

long long getWinCpuUsage() 
{
	FILETIME preidleTime;
	FILETIME prekernelTime;
	FILETIME preuserTime;
	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;

	if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return -1;
	preidleTime = idleTime;
	prekernelTime = kernelTime;
	preuserTime = userTime;	

	Sleep(1000);
	if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return -1;

	const __int64 idle = compareFileTime(idleTime, preidleTime);
	const __int64 kernel = compareFileTime(kernelTime, prekernelTime);
	const __int64 user = compareFileTime(userTime, preuserTime);

	return (kernel + user - idle) * 1000 / (kernel + user);
}

long long getProcessCpu()
{
	const HANDLE hProcess = GetCurrentProcess();
	//if (INVALID_HANDLE_VALUE == hProcess){return -1;}

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	const __int64 iCpuNum = si.dwNumberOfProcessors;

	FILETIME ftPreKernelTime, ftPreUserTime;
	FILETIME ftKernelTime, ftUserTime;
	FILETIME ftCreationTime, ftExitTime;
	std::ofstream log("System.log");

	if (!GetProcessTimes(hProcess, &ftCreationTime, &ftExitTime, &ftPreKernelTime, &ftPreUserTime)) { return -1; }
	log << ftPreKernelTime.dwLowDateTime << "\n" << ftPreUserTime.dwLowDateTime << "\n";
	Sleep(1000);
	if (!GetProcessTimes(hProcess, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime)) { return -1; }
	log << ftKernelTime.dwLowDateTime << "\n" << ftUserTime.dwLowDateTime << "\n";
	const __int64 ullKernelTime = compareFileTime(ftKernelTime, ftPreKernelTime);
	const __int64 ullUserTime = compareFileTime(ftUserTime, ftPreUserTime);
	log << ullKernelTime << "\n" << ullUserTime << "\n" << iCpuNum;
	return (ullKernelTime + ullUserTime) / (iCpuNum * 10);
}

//获取空闲硬盘(千分比)
long long getDiskUsage(double& mbFreeBytes, double& mbTotalBytes){
	/*int sizStr = GetLogicalDriveStrings(0, NULL);//获得本地所有盘符存在Drive数组中
	char* chsDrive = new char[sizStr];//初始化数组用以存储盘符信息
	GetLogicalDriveStrings(sizStr, chsDrive);
	int DType;
	int si = 0;*/
	BOOL fResult;
	unsigned _int64 i64FreeBytesToCaller;
	unsigned _int64 i64TotalBytes;
	unsigned _int64 i64FreeBytes;
	fResult = GetDiskFreeSpaceEx(
		NULL,
		(PULARGE_INTEGER)&i64FreeBytesToCaller,
		(PULARGE_INTEGER)&i64TotalBytes,
		(PULARGE_INTEGER)&i64FreeBytes);
	//GetDiskFreeSpaceEx函数，可以获取驱动器磁盘的空间状态,函数返回的是个BOOL类型数据
	if (fResult) {
		mbTotalBytes = i64TotalBytes * 1000 / 1024 / 1024 / 1024 / 1000.0;//磁盘总容量
		mbFreeBytes = i64FreeBytesToCaller * 1000 / 1024 / 1024 / 1024 / 1000.0;//磁盘剩余空间
		return 1000 - 1000 * i64FreeBytesToCaller / i64TotalBytes;
	}
	return 0;
}

#endif