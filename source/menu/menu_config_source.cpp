
#include "menu.hpp"

bool cur_smallbox;
bool cur_box_mode;

void CMenu::_showConfigSource(bool instant)
{
	_hideCheckboxes(true); // reset checkboxes
	
	m_btnMgr.setText(m_configLblTitle, _t("cfg796", L"Source menu options"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.show(m_configBtnBack);

	for(u8 i = 0; i < ARRAY_SIZE(m_configLblUser); ++i)
		if(m_configLblUser[i] != -1)
			m_btnMgr.show(m_configLblUser[i]);
	
	//! Source menu on start
	m_btnMgr.setText(m_configLbl[1], _t("cfg718", L"Source menu on start"));
	m_checkboxBtn[1] = m_cfg.getOptBool(general_domain, "source_on_start", 0) == 0 ? m_configChkOff[1] : m_configChkOn[1];
	//! Enable sourceflow
	m_btnMgr.setText(m_configLbl[2], _t("cfgsm3", L"Enable sourceflow")); // default false
	m_checkboxBtn[2] = SF_enabled ? m_configChkOn[2] : m_configChkOff[2];
	//! Sourceflow cover box mode
	m_btnMgr.setText(m_configLbl[3], _t("cfg726", L"Covers box mode")); // default false
	m_checkboxBtn[3] = cur_box_mode ? m_configChkOn[3] : m_configChkOff[3];
	//! Sourceflow smallbox
	m_btnMgr.setText(m_configLbl[4], _t("cfgsm4", L"Sourceflow smallbox")); // default true
	m_checkboxBtn[4] = cur_smallbox ? m_configChkOn[4] : m_configChkOff[4];
	//! Sort sourceflow
	m_btnMgr.setText(m_configLbl[5], _t("cfg811", L"Sort sourceflow alphabetically"));
	m_checkboxBtn[5] = m_cfg.getInt(sourceflow_domain, "sort", SORT_ALPHA) == SORT_ALPHA ? m_configChkOn[5] : m_configChkOff[5];
	//! Adjust sourceflow coverflow
	m_btnMgr.setText(m_configLbl[6], _t("cfgc4", L"Adjust coverflow"));
	//! Hide source buttons
	m_btnMgr.setText(m_configLbl[7], _t("smedit1", L"Hide source buttons"));
	//! Link source buttons to plugins
	m_btnMgr.setText(m_configLbl[8], _t("smedit6", L"Link source buttons to plugins"));


	for(u8 i = 1; i < 9; ++i)
	{
		m_btnMgr.show(m_configLbl[i], instant);
		if(i < 6)
			m_btnMgr.show(m_checkboxBtn[i], instant);
		else
			m_btnMgr.show(m_configBtnGo[i], instant);
	}
}

void CMenu::_configSource(void)
{
	if(!m_use_source)
	{
		_error(_t("cfgsmerr", L"No source menu found!"));
		return;
	}

	bool prev_SF_enabled = SF_enabled;
	bool prev_smallbox = m_cfg.getBool(sourceflow_domain, "smallbox", true);
	cur_smallbox = prev_smallbox;
	bool prev_box_mode = m_cfg.getBool(sourceflow_domain, "box_mode", false);
	cur_box_mode = prev_box_mode;
	bool sm_edit = false;

	SetupInput();
	_showConfigSource();

	while(!m_exit)
	{
		_mainLoopCommon(m_sourceflow);
		if(BTN_HOME_HELD || BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
				break;

			if(m_btnMgr.selected(m_checkboxBtn[1])) // SM on start
			{
				m_cfg.setBool(general_domain, "source_on_start", !m_cfg.getBool(general_domain, "source_on_start"));
				_showConfigSource(true);
				m_btnMgr.setSelected(m_checkboxBtn[1]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[2])) // enable SF
			{
				SF_enabled = !SF_enabled;
				m_cfg.setBool(sourceflow_domain, "enabled", SF_enabled);
				_showConfigSource(true);
				m_btnMgr.setSelected(m_checkboxBtn[2]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[3])) // box mode
			{
				cur_box_mode = !cur_box_mode;
				m_cfg.setBool(sourceflow_domain, "box_mode", cur_box_mode);
				_showConfigSource(true);
				m_btnMgr.setSelected(m_checkboxBtn[3]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[4])) // smallbox
			{
				cur_smallbox = !cur_smallbox;
				m_cfg.setBool(sourceflow_domain, "smallbox", cur_smallbox);
				_showConfigSource(true);
				m_btnMgr.setSelected(m_checkboxBtn[4]);
			}
			else if(m_btnMgr.selected(m_checkboxBtn[5])) // sourceflow alpha
			{
				int val = m_cfg.getInt(sourceflow_domain, "sort", SORT_ALPHA);
				m_cfg.setInt(sourceflow_domain, "sort", val == SORT_ALPHA ? SORT_BTN_NUMBERS : SORT_ALPHA);
				if(m_sourceflow)
				{
					_initCF(); // set coverflow to new sorting
					CoverFlow._setCurPos(0); // force first cover of new sort as coverflow current position
				}
				_showConfigSource(true);
				m_btnMgr.setSelected(m_checkboxBtn[5]);
			}
			else if(m_btnMgr.selected(m_configBtnGo[6])) // adjust CF
			{
				_hideConfig();
				bool prev_sourceflow = m_sourceflow;
				m_sourceflow = true;
				_showCF(true);
				if(prev_sourceflow)
					CoverFlow.fade(0);
				if(_cfTheme())
					break; // reboot if CF was modified due to possible memory leak with cf_theme
				m_sourceflow = prev_sourceflow;
				_showCF(true);
				if(prev_sourceflow)
					CoverFlow.fade(2);
				_hideMain(true); // quick fix
				_showConfigSource();
			}
			else if(m_btnMgr.selected(m_configBtnGo[7])) // hide source buttons
			{
				_hideConfig(true);
				_srcTierBack(true);
				_checkboxesMenu(1); // SM editor mode 1 (HIDE_SOURCES)
				sm_edit = true;
				_showConfigSource();
			}
			else if(m_btnMgr.selected(m_configBtnGo[8])) // link source buttons to plugins
			{
				_hideConfig(true);
				_srcTierBack(true);
				_checkboxesMenu(2); // SM editor mode 2 (SELECT_BUTTON)
				sm_edit = true;
				_showConfigSource();
			}
		}
	}

	if(sm_edit || (prev_SF_enabled != SF_enabled) || (cur_smallbox != prev_smallbox) || (cur_box_mode != prev_box_mode))		
	{
		m_cfg.remove(sourceflow_domain, "numbers");
		m_cfg.remove(sourceflow_domain, "tiers");
		m_cfg.removeDomain("SOURCEFLOW_CACHE");
		sm_tier = true; // will force _srcTierBack() and reload ini file
		_srcTierBack(true);
		if(SF_enabled)
			m_refreshGameList = true;
	}
	
	_hideConfig(true);
}
