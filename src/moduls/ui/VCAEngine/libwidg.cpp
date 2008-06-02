
//OpenSCADA system module UI.VCAEngine file: libwidg.cpp
/***************************************************************************
 *   Copyright (C) 2006-2007 by Roman Savochenko                           *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <tsys.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "vcaengine.h"
#include "libwidg.h"

using namespace VCA;

//************************************************
//* WidgetLib: Widgets library                   *
//************************************************
WidgetLib::WidgetLib( const string &id, const string &name, const string &lib_db ) :
    TConfig(&mod->elWdgLib()), m_enable(false), m_id(cfg("ID").getSd()), m_name(cfg("NAME").getSd()),
    m_descr(cfg("DESCR").getSd()), m_ico(cfg("ICO").getSd()), m_dbt(cfg("DB_TBL").getSd()),
    m_user(cfg("USER").getSd()), m_grp(cfg("GRP").getSd()), m_permit(cfg("PERMIT").getId()),
    work_lib_db(lib_db)
{
    m_id = id;
    m_name = name;
    m_dbt = string("wlb_")+id;
    m_wdg = grpAdd("wdg_",(id=="originals")?true:false);
}

WidgetLib::~WidgetLib( )
{

}

TCntrNode &WidgetLib::operator=( TCntrNode &node )
{
    WidgetLib *src_n = dynamic_cast<WidgetLib*>(&node);
    if( !src_n ) return *this;

    //- Copy generic configuration -
    string tid = m_id;
    *(TConfig*)this = *(TConfig*)src_n;
    m_id  = tid;
    m_dbt = string("wlb_")+tid;
    work_lib_db = src_n->work_lib_db;

    if( !src_n->enable() ) return *this;
    if( !enable() ) setEnable(true);

    vector<string> pls;
    //- Mime data copy -
    src_n->mimeDataList(pls);
    string mimeType, mimeData;
    for( int i_m = 0; i_m < pls.size(); i_m++ )
    {
	src_n->mimeDataGet( pls[i_m], mimeType, &mimeData );
	mimeDataSet( pls[i_m], mimeType, mimeData );
    }

    //- Copy include pages -
    src_n->list(pls);
    for( int i_p = 0; i_p < pls.size(); i_p++ )
    {
	if( !present(pls[i_p]) ) add(pls[i_p],"");
	(TCntrNode&)at(pls[i_p]).at() = (TCntrNode&)src_n->at(pls[i_p]).at();
    }

    return *this;
}

void WidgetLib::postEnable( int flag )
{
    if( flag&TCntrNode::NodeRestore )	setEnable(true);
}

void WidgetLib::preDisable( int flag )
{
    if( enable() )  setEnable(false);
}

void WidgetLib::postDisable( int flag )
{
    if( flag )
    {
	//- Delete libraries record -
	SYS->db().at().dataDel(DB()+"."+mod->wlbTable(),mod->nodePath()+"LIB/",*this);

	//- Delete function's files -
	//-- Delete widgets table --
	SYS->db().at().open(fullDB());
	SYS->db().at().close(fullDB(),true);
	//-- Delete attributes table --
	SYS->db().at().open(fullDB()+"_io");
	SYS->db().at().close(fullDB()+"_io",true);
	//-- Delete user attributes table --
	SYS->db().at().open(fullDB()+"_uio");
	SYS->db().at().close(fullDB()+"_uio",true);
	//-- Delete include widgets table --
	SYS->db().at().open(fullDB()+"_incl");
	SYS->db().at().close(fullDB()+"_incl",true);
	//-- Delete mime-data table --
	SYS->db().at().open(fullDB()+"_mime");
	SYS->db().at().close(fullDB()+"_mime",true);
    }
}

string WidgetLib::name( )
{
    return (m_name.size())?m_name:m_id;
}

string WidgetLib::user( )
{
    return SYS->security().at().usrPresent(m_user)?m_user:"root";
}

string WidgetLib::grp( )
{
    return SYS->security().at().grpPresent(m_grp)?m_grp:"UI";
}

void WidgetLib::setUser( const string &it )
{
    m_user = it;
    //- Update librarie's group -
    if(SYS->security().at().grpAt("UI").at().user(it))
	setGrp("UI");
    else
    {
	vector<string> gls;
	SYS->security().at().usrGrpList(user(),gls);
	setGrp(gls.size()?gls[0]:"UI");
    }
    modif();
}

void WidgetLib::setFullDB( const string &it )
{
    work_lib_db = TSYS::strSepParse(it,0,'.')+"."+TSYS::strSepParse(it,1,'.');
    m_dbt = TSYS::strSepParse(it,2,'.');
    modifG();
}

void WidgetLib::load_( )
{
#if OSC_DEBUG
    mess_debug(nodePath().c_str(),_("Load widget library."));
#endif

    SYS->db().at().dataGet(DB()+"."+mod->wlbTable(),mod->nodePath()+"LIB/",*this);

    //- Create new widgets -
    TConfig c_el(&mod->elWdg());
    c_el.cfgViewAll(false);
    int fld_cnt = 0;
    while( SYS->db().at().dataSeek(fullDB(),mod->nodePath()+tbl(), fld_cnt++,c_el) )
    {
	string f_id = c_el.cfg("ID").getS();
	c_el.cfg("ID").setS("");
	if( !present(f_id) )	add(f_id,"","");
    }

    mOldDB = TBDS::realDBName(DB());
}

void WidgetLib::save_( )
{
#if OSC_DEBUG
    mess_debug(nodePath().c_str(),_("Save widget library."));
#endif

    SYS->db().at().dataSet(DB()+"."+mod->wlbTable(),mod->nodePath()+"LIB/",*this);

    //- Check for need copy mime data to other DB and same copy -
    if( !mOldDB.empty() && mOldDB != TBDS::realDBName(DB()) )
    {
	vector<string> pls;
	mimeDataList(pls,mOldDB);
	string mimeType, mimeData;
	for( int i_m = 0; i_m < pls.size(); i_m++ )
	{
	    mimeDataGet( pls[i_m], mimeType, &mimeData, mOldDB );
	    mimeDataSet( pls[i_m], mimeType, mimeData, DB() );
	}
    }

    mOldDB = TBDS::realDBName(DB());
}

void WidgetLib::setEnable( bool val )
{
    if( val == enable() )       return;

#if OSC_DEBUG
    mess_debug(nodePath().c_str(),val ? _("Enable widget library.") : _("Disable widget library."));
#endif

    vector<string> f_lst;
    list(f_lst);
    for( int i_ls = 0; i_ls < f_lst.size(); i_ls++ )
	try { at(f_lst[i_ls]).at().setEnable(val); }
	catch( TError err )
	{ mess_err(nodePath().c_str(),_("Enable/disable widget '%s' error %s."),f_lst[i_ls].c_str(),err.mess.c_str()); }

    m_enable = val;
}

void WidgetLib::mimeDataList( vector<string> &list, const string &idb )
{
    string wtbl = tbl()+"_mime";
    string wdb  = idb.empty() ? DB() : idb;
    TConfig c_el( &mod->elWdgData() );
    c_el.cfgViewAll( false );

    list.clear( );
    for( int fld_cnt = 0; SYS->db().at().dataSeek(wdb+"."+wtbl,mod->nodePath()+wtbl,fld_cnt,c_el); fld_cnt++ )
    {
	list.push_back( c_el.cfg("ID").getS() );
	c_el.cfg("ID").setS("");
    }
}

bool WidgetLib::mimeDataGet( const string &iid, string &mimeType, string *mimeData, const string &idb )
{
    bool is_file = (iid.size()>5 && iid.substr(0,5) == "file:");
    bool is_res  = (iid.size()>4 && iid.substr(0,4) == "res:");

    if( !is_file )
    {
	//- Get resource file from DB -
	string dbid = is_res ? iid.substr(4) : iid;
	string wtbl = tbl()+"_mime";
	string wdb  = idb.empty() ? DB() : idb;	
	TConfig c_el( &mod->elWdgData() );
	if( !mimeData ) c_el.cfg("DATA").setView(false);
	c_el.cfg("ID").setS( dbid );
	if( SYS->db().at().dataGet( wdb+"."+wtbl, mod->nodePath()+wtbl, c_el ) )
	{
	    mimeType = c_el.cfg("MIME").getS();
	    if( mimeData )	*mimeData = c_el.cfg("DATA").getS();
	    return true;
	}
    }
    if( !is_res )
    {
	//- Get resource file from file system -
	string filepath = is_file ? iid.substr(5) : iid;
	int len;
	char buf[STR_BUF_LEN];
	string rez;
	int hd = open(filepath.c_str(),O_RDONLY);
	if( hd == -1 )	return false;

	while( len = read(hd,buf,sizeof(buf)) ) rez.append(buf,len);
	close(hd);

	mimeType = ((filepath.rfind(".") != string::npos) ? filepath.substr(filepath.rfind(".")+1)+";" : "file/unknown;")+TSYS::int2str(rez.size());
	if( mimeData )	*mimeData = TSYS::strEncode(rez,TSYS::base64);
	return true;
    }

    return false;
}

void WidgetLib::mimeDataSet( const string &iid, const string &mimeType, const string &mimeData, const string &idb )
{
    string wtbl = tbl()+"_mime";
    string wdb  = idb.empty() ? DB() : idb;
    TConfig c_el( &mod->elWdgData() );
    c_el.cfg("ID").setS( iid );
    c_el.cfg("MIME").setS( mimeType );
    if( !mimeData.size() ) c_el.cfg("DATA").setView(false);
    else c_el.cfg("DATA").setS(mimeData);
    SYS->db().at().dataSet( wdb+"."+wtbl, mod->nodePath()+wtbl, c_el );
}

void WidgetLib::mimeDataDel( const string &iid, const string &idb )
{
    string wtbl = tbl()+"_mime";
    string wdb  = idb.empty() ? DB() : idb;
    TConfig c_el( &mod->elWdgData() );
    c_el.cfg("ID").setS(iid);
    SYS->db().at().dataDel( wdb+"."+wtbl, mod->nodePath()+wtbl, c_el );
}

void WidgetLib::add( const string &id, const string &name, const string &orig )
{
    if( present(id) )	return;
    chldAdd(m_wdg,new LWidget(id,orig));
    at(id).at().setName(name);
}

void WidgetLib::add( LWidget *iwdg )
{
    if( present(iwdg->id()) )	delete iwdg;
    else chldAdd(m_wdg,iwdg);
}

AutoHD<LWidget> WidgetLib::at( const string &id )
{
    return chldAt(m_wdg,id);
}

void WidgetLib::cntrCmdProc( XMLNode *opt )
{
    //- Get page info -
    if( opt->name() == "info" )
    {
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Widget's library: ")+id(),permit(),user().c_str(),grp().c_str());
	if(ico().size()) ctrMkNode("img",opt,-1,"/ico","",R_R_R_);
	if(ctrMkNode("branches",opt,-1,"/br","",R_R_R_))
	    ctrMkNode("grp",opt,-1,"/br/wdg_",_("Widget"),permit(),user().c_str(),grp().c_str(),1,"idm","1");
	if(ctrMkNode("area",opt,-1,"/obj",_("Library")))
	{
	    if(ctrMkNode("area",opt,-1,"/obj/st",_("State")))
	    {
		ctrMkNode("fld",opt,-1,"/obj/st/en",_("Enable"),RWRWR_,user().c_str(),grp().c_str(),1,"tp","bool");
		ctrMkNode("fld",opt,-1,"/obj/st/db",_("Library DB"),RWR_R_,user().c_str(),grp().c_str(),2,
		    "tp","str","help",_("DB address in format [<DB module>.<DB name>.<Table name>].\nFor use main work DB set symbol '*'."));
		ctrMkNode("fld",opt,-1,"/obj/st/user",_("User and group"),RWRWR_,"root","root",3,"tp","str","dest","select","select","/obj/u_lst");
		ctrMkNode("fld",opt,-1,"/obj/st/grp","",RWR_R_,user().c_str(),grp().c_str(),3,"tp","str","dest","select","select","/obj/g_lst");
	    }
	    if(ctrMkNode("area",opt,-1,"/obj/cfg",_("Config")))
	    {
		ctrMkNode("fld",opt,-1,"/obj/cfg/id",_("Id"),R_R_R_,user().c_str(),grp().c_str(),1,"tp","str");
		ctrMkNode("fld",opt,-1,"/obj/cfg/name",_("Name"),permit(),user().c_str(),grp().c_str(),1,"tp","str");
		ctrMkNode("fld",opt,-1,"/obj/cfg/descr",_("Description"),permit(),user().c_str(),grp().c_str(),3,"tp","str","cols","50","rows","3");
		ctrMkNode("img",opt,-1,"/obj/cfg/ico",_("Icon"),permit(),user().c_str(),grp().c_str(),2,"v_sz","64","h_sz","64");
		ctrMkNode("fld",opt,-1,"/obj/cfg/u_a",_("Access: user-grp-other"),RWR_R_,user().c_str(),grp().c_str(),3,"tp","dec","dest","select","select","/obj/a_lst");
		ctrMkNode("fld",opt,-1,"/obj/cfg/g_a","",RWR_R_,user().c_str(),grp().c_str(),3,"tp","dec","dest","select","select","/obj/a_lst");
		ctrMkNode("fld",opt,-1,"/obj/cfg/o_a","",RWR_R_,user().c_str(),grp().c_str(),3,"tp","dec","dest","select","select","/obj/a_lst");
	    }
	}
	if(ctrMkNode("area",opt,-1,"/wdg",_("Widgets")))
	    ctrMkNode("list",opt,-1,"/wdg/wdg",_("Widgets"),permit(),user().c_str(),grp().c_str(),4,"tp","br","idm","1","s_com","add,del","br_pref","wdg_");
	if(ctrMkNode("area",opt,-1,"/mime",_("Mime data")))
	    if(ctrMkNode("table",opt,-1,"/mime/mime",_("Mime data"),permit(),user().c_str(),grp().c_str(),2,"s_com","add,del","key","id"))
	    {
		ctrMkNode("list",opt,-1,"/mime/mime/id",_("Id"),permit(),user().c_str(),grp().c_str(),1,"tp","str");
		ctrMkNode("list",opt,-1,"/mime/mime/tp",_("Mime type"),permit(),user().c_str(),grp().c_str(),1,"tp","str");
		ctrMkNode("list",opt,-1,"/mime/mime/dt",_("Data"),permit(),user().c_str(),grp().c_str(),2,"tp","str","dest","data");
	    }
        return;
    }

    //- Process command to page -
    string a_path = opt->attr("path");
    if( a_path == "/obj/st/en" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,user().c_str(),grp().c_str(),SEQ_RD) ) opt->setText( TSYS::int2str(enable()) );
	if( ctrChkNode(opt,"set",RWRWR_,user().c_str(),grp().c_str(),SEQ_WR) ) setEnable( atoi(opt->text().c_str()) );
    }
    else if( a_path == "/obj/st/db" )
    {
	if( ctrChkNode(opt,"get",RWR_R_,user().c_str(),grp().c_str(),SEQ_RD) )	opt->setText( fullDB() );
	if( ctrChkNode(opt,"set",RWR_R_,user().c_str(),grp().c_str(),SEQ_WR) )	setFullDB( opt->text() );
    }
    else if( a_path == "/obj/st/user" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","root",SEQ_RD) )	opt->setText( user() );
	if( ctrChkNode(opt,"set",RWRWR_,"root","root",SEQ_WR) )	setUser( opt->text() );
    }
    else if( a_path == "/obj/st/grp" )
    {
	if( ctrChkNode(opt,"get",RWR_R_,user().c_str(),grp().c_str(),SEQ_RD) )	opt->setText( grp() );
	if( ctrChkNode(opt,"set",RWR_R_,user().c_str(),grp().c_str(),SEQ_WR) )	setGrp( opt->text() );
    }
    else if( a_path == "/obj/cfg/u_a" || a_path == "/obj/cfg/g_a" || a_path == "/obj/cfg/o_a" )
    {
	if( ctrChkNode(opt,"get",RWR_R_,user().c_str(),grp().c_str(),SEQ_RD) )
	{
	    if( a_path == "/obj/cfg/u_a" )	opt->setText( TSYS::int2str((permit()>>6)&0x7) );
	    if( a_path == "/obj/cfg/g_a" )	opt->setText( TSYS::int2str((permit()>>3)&0x7) );
	    if( a_path == "/obj/cfg/o_a" )	opt->setText( TSYS::int2str(permit()&0x7) );
	}
	if( ctrChkNode(opt,"set",RWR_R_,user().c_str(),grp().c_str(),SEQ_WR) )
	{
	    if( a_path == "/obj/cfg/u_a" )	setPermit( (permit()&(~(0x07<<6)))|(atoi(opt->text().c_str())<<6) );
	    if( a_path == "/obj/cfg/g_a" )	setPermit( (permit()&(~(0x07<<3)))|(atoi(opt->text().c_str())<<3) );
	    if( a_path == "/obj/cfg/o_a" )	setPermit( (permit()&(~0x07))|atoi(opt->text().c_str()) );
	}
    }
    else if( a_path == "/obj/cfg/ico" || a_path == "/ico" )
    {
	if( ctrChkNode(opt,"get",permit(),user().c_str(),grp().c_str(),SEQ_RD) )   opt->setText( ico() );
	if( ctrChkNode(opt,"set",permit(),user().c_str(),grp().c_str(),SEQ_WR) )   setIco( opt->text() );
    }
    else if( a_path == "/obj/cfg/id" && ctrChkNode(opt,"get",R_R_R_,user().c_str(),grp().c_str()) ) opt->setText(id());
    else if( a_path == "/obj/cfg/name" )
    {
	if( ctrChkNode(opt,"get",permit(),user().c_str(),grp().c_str(),SEQ_RD) )   opt->setText( name() );
	if( ctrChkNode(opt,"set",permit(),user().c_str(),grp().c_str(),SEQ_WR) )   setName( opt->text() );
    }
    else if( a_path == "/obj/cfg/descr" )
    {
	if( ctrChkNode(opt,"get",permit(),user().c_str(),grp().c_str(),SEQ_RD) )   opt->setText( descr() );
	if( ctrChkNode(opt,"set",permit(),user().c_str(),grp().c_str(),SEQ_WR) )   setDescr( opt->text() );
    }
    else if( a_path == "/br/wdg_" || a_path == "/wdg/wdg" )
    {
	if( ctrChkNode(opt,"get",permit(),user().c_str(),grp().c_str(),SEQ_RD) )
	{
	    vector<string> lst;
	    list(lst);
	    for( unsigned i_f=0; i_f < lst.size(); i_f++ )
		opt->childAdd("el")->setAttr("id",lst[i_f])->setText(at(lst[i_f]).at().name());
	}
	if( ctrChkNode(opt,"add",permit(),user().c_str(),grp().c_str(),SEQ_WR) )
	{
	    add(opt->attr("id").c_str(),opt->text().c_str());
	    at(opt->attr("id")).at().setUser(opt->attr("user"));
	}
	if( ctrChkNode(opt,"del",permit(),user().c_str(),grp().c_str(),SEQ_WR) ) del(opt->attr("id"),true);
    }
    else if( a_path == "/obj/u_lst" && ctrChkNode(opt) )
    {
	vector<string> ls;
	SYS->security().at().usrList(ls);
	for(int i_l = 0; i_l < ls.size(); i_l++)
		opt->childAdd("el")->setText(ls[i_l]);
    }
    else if( a_path == "/obj/g_lst" && ctrChkNode(opt) )
    {
	vector<string> ls;
	SYS->security().at().usrGrpList(user(), ls );
	for(int i_l = 0; i_l < ls.size(); i_l++)
	    opt->childAdd("el")->setText(ls[i_l]);
    }
    else if( a_path == "/obj/a_lst" && ctrChkNode(opt) )
    {
	opt->childAdd("el")->setAttr("id","0")->setText(_("No access"));
	opt->childAdd("el")->setAttr("id","4")->setText(_("Use(open)"));
	opt->childAdd("el")->setAttr("id","2")->setText(_("Modify"));
	opt->childAdd("el")->setAttr("id","6")->setText(_("Full"));
    }
    else if( a_path == "/mime/mime" )
    {
	//-- Request data --
	string idmime = opt->attr("key_id");
	string idcol  = opt->attr("col");

	if( ctrChkNode(opt,"get",permit(),user().c_str(),grp().c_str(),SEQ_RD) )
	{
	    if( !idmime.empty() && idcol == "dt" && atoi(opt->attr("data").c_str()) )
	    {
		string mimeType, mimeData;
		if( mimeDataGet( "res:"+idmime, mimeType, &mimeData ) ) opt->setText( mimeData );
	    }
	    else
	    {
		XMLNode *n_id = ctrMkNode("list",opt,-1,"/mime/mime/id","");
		XMLNode *n_tp = ctrMkNode("list",opt,-1,"/mime/mime/tp","");
		XMLNode *n_dt = ctrMkNode("list",opt,-1,"/mime/mime/dt","");

		vector<string> lst;
		string mimeType;
		mimeDataList(lst);
		for(int i_el = 0; i_el < lst.size(); i_el++)
		    if( mimeDataGet("res:"+lst[i_el],mimeType) )
		    {
			if( n_id )	n_id->childAdd("el")->setText(lst[i_el]);
			if( n_tp )	n_tp->childAdd("el")->setText(TSYS::strSepParse(mimeType,0,';'));
			if( n_dt )	n_dt->childAdd("el")->setText(TSYS::strSepParse(mimeType,1,';'));
		    }
	    }
	}
	if( ctrChkNode(opt,"add",permit(),user().c_str(),grp().c_str(),SEQ_WR) )
	    mimeDataSet("newMime","image/new;0","");
	if( ctrChkNode(opt,"del",permit(),user().c_str(),grp().c_str(),SEQ_WR) )
	    mimeDataDel(opt->attr("key_id"));
	if( ctrChkNode(opt,"set",permit(),user().c_str(),grp().c_str(),SEQ_WR) )
	{
	    //-- Request data --
	    if( idcol == "id" )
	    {
		string mimeType, mimeData;
		//--- Copy mime data to new record ---
		if( mimeDataGet( "res:"+idmime, mimeType, &mimeData ) )
		{
		    mimeDataSet( opt->text(), mimeType, mimeData );
		    mimeDataDel( idmime );
		}
	    }
	    else if( idcol == "tp" )
	    {
		string mimeType;
		//--- Copy mime data to new record ---
		if( mimeDataGet( "res:"+idmime, mimeType ) )
		    mimeDataSet( idmime, opt->text()+";"+TSYS::strSepParse(mimeType,1,';'), "");
	    }
	    else if( idcol == "dt" )
	    {
		string mimeType;
		if( mimeDataGet( "res:"+idmime, mimeType ) )
		    mimeDataSet( idmime, TSYS::strSepParse(mimeType,0,';')+";"+TSYS::real2str((float)opt->text().size()/1024.,6),opt->text() );
	    }
	}
    }
    else TCntrNode::cntrCmdProc(opt);
}

//************************************************
//* LWidget: Library stored widget               *
//************************************************
LWidget::LWidget( const string &iid, const string &isrcwdg ) :
	Widget(iid), TConfig(&mod->elWdg()),
	m_ico(cfg("ICO").getSd()), m_proc(cfg("PROC").getSd()), m_proc_per(cfg("PROC_PER").getId()),
	m_user(cfg("USER").getSd()), m_grp(cfg("GRP").getSd()), m_permit(cfg("PERMIT").getId()), 
	m_parent(cfg("PARENT").getSd()), m_attrs(cfg("ATTRS").getSd())
{
    cfg("ID").setS(id());

    setParentNm(isrcwdg);
}

LWidget::~LWidget( )
{

}

WidgetLib &LWidget::owner( )
{
    return *(WidgetLib*)nodePrev( );
}

void LWidget::postDisable( int flag )
{

}

string LWidget::path( )
{
    return "/wlb_"+owner().id()+"/wdg_"+id();
}

string LWidget::ico( )
{
    if( m_ico.size() )          return m_ico;
    if( !parent().freeStat() )  return parent().at().ico();
    return "";
}

string LWidget::user( )
{
    return SYS->security().at().usrPresent(m_user)?m_user:Widget::user( );
}

void LWidget::setUser( const string &iuser )
{
    m_user = iuser;
    //- Group update -
    if(SYS->security().at().grpAt("UI").at().user(iuser))
	setGrp("UI");
    else
    {
	vector<string> gls;
	SYS->security().at().usrGrpList(user(),gls);
	setGrp(gls.size()?gls[0]:Widget::grp());
    }
    modif();
}

string LWidget::grp( )
{
    return SYS->security().at().grpPresent(m_grp)?m_grp:Widget::grp( );
}

string LWidget::calcId( )
{
    if( m_proc.empty() )
    {
	if( !parent().freeStat() ) return parent().at().calcId( );
	return "";
    }
    return "L_"+owner().id()+"_"+id();
}

string LWidget::calcLang( )
{
    if( !m_proc.size() && !parent().freeStat() )
	return parent().at().calcLang();

    string iprg = m_proc;
    if(iprg.find("\n") == string::npos)
    {
	iprg = iprg+"\n";
	m_proc = iprg;
    }
    return iprg.substr(0,iprg.find("\n"));
}

string LWidget::calcProg( )
{
    if( !m_proc.size() && !parent().freeStat() )
	return parent().at().calcProg();

    string iprg = m_proc;
    int lng_end = iprg.find("\n");
    if(lng_end == string::npos) lng_end=0;
    else lng_end++;
    return iprg.substr(lng_end);
}

int LWidget::calcPer( )
{
    if( m_proc_per < 0 && !parent().freeStat() )
	return parent().at().calcPer();
    return m_proc_per;
}

void LWidget::setCalcLang( const string &ilng )
{
    m_proc = m_proc.replace(0,m_proc.find("\n"),ilng);
    modif();
}

void LWidget::setCalcProg( const string &iprg )
{
    string tmp_prg = m_proc;
    int lng_end = tmp_prg.find("\n");
    if(lng_end == string::npos)
    {
	tmp_prg = tmp_prg+"\n";
	lng_end = tmp_prg.find("\n");
    }
    m_proc = tmp_prg.replace(lng_end+1,string::npos,iprg);
    modif();
}

void LWidget::setCalcPer( int vl )
{
    m_proc_per = vl;
    modif();
}

void LWidget::load_( )
{
    //- Load generic widget's data -
    string db  = owner().DB();
    string tbl = owner().tbl();
    SYS->db().at().dataGet( db+"."+tbl, mod->nodePath()+tbl, *this );

    loadIO();
}

void LWidget::loadIO( )
{
    string tstr;
    vector<string> als;

    if( !enable() ) return;

    //- Load widget's work attributes -
    string db  = owner().DB();
    string tbl = owner().tbl()+"_io";

    //- Inherit modify attributes -
    /*attrList( als );
    for( int i_a = 0; i_a < als.size(); i_a++ )
    {
        AutoHD<Attr> attr = attrAt(als[i_a]);
        if( attr.at().flgGlob()&Attr::IsInher && attr.at().modif() && m_attrs.find(als[i_a]) == string::npos )
        {
            attr.at().setModif(0);
            inheritAttr(als[i_a]);
        }
    }
    als.clear();*/
    bool full_ls = false;
    if( m_attrs == "*" )	{ attrList( als ); full_ls = true; }
    else
	for( int off = 0; !(tstr = TSYS::strSepParse(m_attrs,0,';',&off)).empty(); )
	    als.push_back(tstr);

    //-- Same attributes load --
    TConfig c_el(&mod->elWdgIO());
    c_el.cfg("IDW").setS(id());
    for( int i_a = 0; i_a < als.size(); i_a++ )
    {
	if( !attrPresent(als[i_a]) )    continue;
	AutoHD<Attr> attr = attrAt(als[i_a]);
	if( !(attr.at().flgGlob()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser ) continue;
	c_el.cfg("ID").setS(als[i_a]);
	if( !SYS->db().at().dataGet(db+"."+tbl,mod->nodePath()+tbl,c_el) ) continue;
	attr.at().setS(c_el.cfg("IO_VAL").getS());
	attr.at().setFlgSelf((Attr::SelfAttrFlgs)c_el.cfg("SELF_FLG").getI());
	attr.at().setCfgTempl(c_el.cfg("CFG_TMPL").getS());
	attr.at().setCfgVal(c_el.cfg("CFG_VAL").getS());
	if( full_ls && attr.at().flgGlob()&Attr::Active ) attrList( als );
    }

    //- Load widget's user attributes -
    tbl = owner().tbl()+"_uio";
    c_el.setElem(&mod->elWdgUIO());
    c_el.cfg("IDW").setS(id());
    c_el.cfg("ID").setS("");
    int fld_cnt = 0;
    while( SYS->db().at().dataSeek(db+"."+tbl,mod->nodePath()+tbl,fld_cnt++,c_el) )
    {
	string sid = c_el.cfg("ID").getS();
	unsigned flg = c_el.cfg("IO_TYPE").getI();
	c_el.cfg("ID").setS("");

	if( !TSYS::pathLev(sid,1).empty() ) continue;
	if( !attrPresent(sid) )
	    attrAdd( new TFld(sid.c_str(),c_el.cfg("NAME").getS().c_str(),(TFld::Type)(flg&0x0f),flg>>4) );
	AutoHD<Attr> attr = attrAt(sid);
	if( !(!(attr.at().flgGlob()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser) ) continue;
	attr.at().setS(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),0,'|'));
	attr.at().fld().setValues(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),1,'|'));
	attr.at().fld().setSelNames(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),2,'|'));
	attr.at().setFlgSelf((Attr::SelfAttrFlgs)c_el.cfg("SELF_FLG").getI());
	attr.at().setCfgTempl(c_el.cfg("CFG_TMPL").getS());
	attr.at().setCfgVal(c_el.cfg("CFG_VAL").getS());
    }

    //- Load cotainer widgets -
    if( !enable() || !isContainer() ) return;
    c_el.setElem(&mod->elInclWdg());
    tbl=owner().tbl()+"_incl";
    c_el.cfg("IDW").setS(id());
    c_el.cfg("ID").setS("");
    fld_cnt=0;
    while( SYS->db().at().dataSeek(db+"."+tbl,mod->nodePath()+tbl,fld_cnt++,c_el) )
    {
	string sid  = c_el.cfg("ID").getS();
	c_el.cfg("ID").setS("");
	if( c_el.cfg("PARENT").getS() == "<deleted>" )
	{
	    if( wdgPresent(sid) )	wdgDel(sid);
	    continue;
	}
	if( !wdgPresent(sid) ) wdgAdd(sid,"","");

	wdgAt(sid).at().load();
    }
}

void LWidget::save_( )
{
    string db  = owner().DB();
    string tbl = owner().tbl();

    //- Remove from DB -
    if( nodeMode() == TCntrNode::Disable )
    {
	//-- Remove from library table --
	SYS->db().at().dataDel( db+"."+tbl, mod->nodePath()+tbl, *this );
	
	//-- Remove widget's IO from library IO table --
	TConfig c_el( &mod->elWdgIO() );
	c_el.cfg("IDW").setS( id() );
	c_el.cfg("ID").setS( "" );
	SYS->db().at().dataDel( db+"."+tbl+"_io", mod->nodePath()+tbl+"_io", c_el );

	//-- Remove widget's user IO from library IO table --
	c_el.setElem( &mod->elWdgUIO() );
	c_el.cfg("IDW").setS( id() );
	c_el.cfg("ID").setS( "" );
	SYS->db().at().dataDel( db+"."+tbl+"_uio", mod->nodePath()+tbl+"_uio", c_el );

	//-- Remove widget's included widgets from library include table --
	c_el.setElem( &mod->elInclWdg() );
	c_el.cfg("IDW").setS( id() );
	c_el.cfg("ID").setS( "" );
	SYS->db().at().dataDel( db+"."+tbl+"_incl", mod->nodePath()+tbl+"_incl", c_el );
    }
    //- Save widget's data -
    else
    {
	//-- Save generic widget's data --
	m_attrs="";
	vector<string> als;
	attrList( als );
	for( int i_a = 0; i_a < als.size(); i_a++ )
	{
	    AutoHD<Attr> attr = attrAt(als[i_a]);
	    if( attr.at().modif() && !(!(attr.at().flgGlob()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser) )
		m_attrs+=als[i_a]+";";
	}
	SYS->db().at().dataSet( db+"."+tbl, mod->nodePath()+tbl, *this );

	//-- Save widget's attributes --
	saveIO();
    }
}

void LWidget::saveIO( )
{
    vector<string> als;

    if( !enable() ) return;

    //- Save widget's attributes -
    string db  = owner().DB();
    string tbl = owner().tbl()+"_io";
    string utbl = owner().tbl()+"_uio";

    attrList( als );
    TConfig c_el(&mod->elWdgIO());
    c_el.cfg("IDW").setS(id());
    TConfig c_elu(&mod->elWdgUIO());
    c_elu.cfg("IDW").setS(id());
    for( int i_a = 0; i_a < als.size(); i_a++ )
    {
	AutoHD<Attr> attr = attrAt(als[i_a]);
	if( !attr.at().modif() )	continue;
	if( !(attr.at().flgGlob()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser )
	{
	    //-- User attribute store --
	    c_elu.cfg("ID").setS(als[i_a]);
	    c_elu.cfg("IO_VAL").setS(attr.at().getS()+"|"+
				    attr.at().fld().values()+"|"+
				    attr.at().fld().selNames());
	    c_elu.cfg("NAME").setS(attr.at().name());
	    c_elu.cfg("IO_TYPE").setI(attr.at().fld().type()+(attr.at().fld().flg()<<4));
	    c_elu.cfg("SELF_FLG").setI(attr.at().flgSelf());
	    c_elu.cfg("CFG_TMPL").setS(attr.at().cfgTempl());
	    c_elu.cfg("CFG_VAL").setS(attr.at().cfgVal());
	    SYS->db().at().dataSet(db+"."+utbl,mod->nodePath()+utbl,c_elu);
	}
	else
	{
	    //-- Work attribute store --
	    c_el.cfg("ID").setS(als[i_a]);
	    c_el.cfg("IO_VAL").setS(attr.at().getS());
	    c_el.cfg("SELF_FLG").setI(attr.at().flgSelf());
	    c_el.cfg("CFG_TMPL").setS(attr.at().cfgTempl());
	    c_el.cfg("CFG_VAL").setS(attr.at().cfgVal());
	    SYS->db().at().dataSet(db+"."+tbl,mod->nodePath()+tbl,c_el);
	}
    }

    //- Clear no present IO for user io table -
    int fld_cnt=0;
    c_elu.cfg("ID").setS("");
    c_elu.cfgViewAll(false);
    while( SYS->db().at().dataSeek(db+"."+utbl,mod->nodePath()+utbl,fld_cnt++,c_elu) )
    {
	string sid = c_elu.cfg("ID").getS();
	if( TSYS::pathLev(sid,1).empty() && !attrPresent(TSYS::pathLev(sid,0)) )
	{
	    SYS->db().at().dataDel(db+"."+utbl,mod->nodePath()+utbl,c_elu);
	    fld_cnt--;
	}
	c_elu.cfg("ID").setS("");
    }
}

void LWidget::wdgAdd( const string &wid, const string &name, const string &path )
{
    if( !isContainer() )  throw TError(nodePath().c_str(),_("No container widget!"));
    if( wdgPresent(wid) ) return;

    //- Check for label <deleted> -
    string db  = owner().DB();
    string tbl = owner().tbl()+"_incl";
    TConfig c_el( &mod->elInclWdg() );
    c_el.cfg("IDW").setS(id());
    c_el.cfg("ID").setS(wid);
    if( SYS->db().at().dataGet( db+"."+tbl, mod->nodePath()+tbl, c_el ) && c_el.cfg("PARENT").getS() == "<deleted>" )
    {
	if( !parent().at().wdgPresent(wid) )	SYS->db().at().dataDel( db+"."+tbl, mod->nodePath()+tbl, c_el );
	return;
    }

    //- Same widget add -
    chldAdd(inclWdg,new CWidget(wid,path));
    wdgAt(wid).at().setName(name);

    //- Call heritors include widgets update -
    for( int i_h = 0; i_h < m_herit.size(); i_h++ )
	if( m_herit[i_h].at().enable( ) )
	    m_herit[i_h].at().inheritIncl(wid);
}

AutoHD<CWidget> LWidget::wdgAt( const string &wdg )
{
    return Widget::wdgAt(wdg);
}

string LWidget::resourceGet( const string &id, string *mime )
{
    string mimeType, mimeData;

    if( !owner().mimeDataGet( id, mimeType, &mimeData ) && !parent().freeStat() )
	mimeData = parent().at().resourceGet( id, &mimeType );
    if( mime )	*mime = mimeType;

    return mimeData;
}

void LWidget::inheritAttr( const string &attr )
{
    bool mdf = isModify();
    Widget::inheritAttr( attr );
    if( !mdf )	modifClr( );
}

void LWidget::cntrCmdProc( XMLNode *opt )
{
    if( cntrCmdServ(opt) ) return;
    //- Get page info -
    if( opt->name() == "info" )
    {
	cntrCmdGeneric(opt);
	cntrCmdAttributes(opt );
	cntrCmdLinks(opt);
	cntrCmdProcess(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Library widget: ")+id());
	return;
    }
    if( !(cntrCmdGeneric(opt) || cntrCmdAttributes(opt) || cntrCmdLinks(opt) || cntrCmdProcess(opt)) )
	TCntrNode::cntrCmdProc(opt);
}

//************************************************
//* CWidget: Container stored widget             *
//************************************************
CWidget::CWidget( const string &iid, const string &isrcwdg ) :
        Widget(iid), TConfig(&mod->elInclWdg()), m_parent(cfg("PARENT").getSd()), m_attrs(cfg("ATTRS").getSd()), delMark(false)
{
    cfg("ID").setS(id());
    m_lnk = true;
    setParentNm(isrcwdg);
}

CWidget::~CWidget( )
{

}

string CWidget::path( )
{
    return "/wlb_"+owner().owner().id()+"/wdg_"+owner().id()+"/wdg_"+id();
}

LWidget &CWidget::owner()
{
    return *(LWidget*)nodePrev();
}

void CWidget::postEnable( int flag )
{
    //- Call parent methos -
    Widget::postEnable(flag);
    //- Set container widget id -
    cfg("IDW").setS(owner().id());
}

void CWidget::preDisable( int flag )
{
    if( flag )	delMark = !((flag>>8)&0x10) && !parent().freeStat() && parent().at().isLink();

    Widget::preDisable( flag );
}

string CWidget::ico( )
{
    if( !parent().freeStat() )  return parent().at().ico();
    return "";
}

string CWidget::user( )
{
    return owner().user();
}

string CWidget::grp( )
{
    return owner().grp( );
}

short CWidget::permit( )
{
    return owner().permit( );
}

void CWidget::setEnable( bool val )
{
    if( enable() == val ) return;

    Widget::setEnable(val);

    //- Disable heritors widgets -
    if( val )
	for( int i_h = 0; i_h < owner().herit().size(); i_h++ )
	    if( owner().herit()[i_h].at().wdgPresent(id()) && !owner().herit()[i_h].at().wdgAt(id()).at().enable( ) )
	    try { owner().herit()[i_h].at().wdgAt(id()).at().setEnable(true); }
	    catch(...)
	    { mess_err(nodePath().c_str(),_("Heritors widget <%s> enable error"),id().c_str()); }
}

string CWidget::calcId( )
{
    if( !parent().freeStat() )	return parent().at().calcId( );
    return "";
}

string CWidget::calcLang( )
{
    if( !parent().freeStat() )	return parent().at().calcLang();
    return "";
}

string CWidget::calcProg( )
{
    if( !parent().freeStat() )	return parent().at().calcProg();
    return "";
}

int CWidget::calcPer( )
{
    if( !parent().freeStat() )	return parent().at().calcPer();
    return 0;
}

void CWidget::load_( )
{
    //- Load generic widget's data -
    string db  = owner().owner().DB();
    string tbl = owner().owner().tbl()+"_incl";
    SYS->db().at().dataGet(db+"."+tbl,mod->nodePath()+tbl,*this);

    //- Load widget's attributes -
    loadIO();
}

void CWidget::loadIO( )
{
    string tstr;
    vector<string> als;

    if( !enable() ) return;

    //- Load widget's work attributes -
    string db  = owner().owner().DB();
    string tbl = owner().owner().tbl()+"_io";

    //- Inherit modify attributes -
    /*attrList( als );
    for( int i_a = 0; i_a < als.size(); i_a++ )
    {
	AutoHD<Attr> attr = attrAt(als[i_a]);
	if( attr.at().flgGlob()&Attr::IsInher && attr.at().modif() && m_attrs.find(als[i_a]) == string::npos )
	{
	    attr.at().setModif(0);
	    inheritAttr(als[i_a]);
	}
    }
    als.clear();*/
    bool full_ls = false;
    if( m_attrs == "*" )	{ attrList( als ); full_ls = true; }
    else
	for( int off = 0; !(tstr = TSYS::strSepParse(m_attrs,0,';',&off)).empty(); )
	    als.push_back(tstr);

    //-- Same load --
    TConfig c_el(&mod->elWdgIO());
    c_el.cfg("IDW").setS(owner().id());
    for( int i_a = 0; i_a < als.size(); i_a++ )
    {
	if( !attrPresent(als[i_a]) )    continue;
	AutoHD<Attr> attr = attrAt(als[i_a]);
	if( !(attr.at().flgGlob()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser ) continue;
	c_el.cfg("ID").setS(id()+"/"+als[i_a]);
	if( !SYS->db().at().dataGet(db+"."+tbl,mod->nodePath()+tbl,c_el) ) continue;
	attr.at().setS(c_el.cfg("IO_VAL").getS());
	attr.at().setFlgSelf((Attr::SelfAttrFlgs)c_el.cfg("SELF_FLG").getI());
	attr.at().setCfgTempl(c_el.cfg("CFG_TMPL").getS());
	attr.at().setCfgVal(c_el.cfg("CFG_VAL").getS());
	if( full_ls && attr.at().flgGlob()&Attr::Active ) attrList( als );
    }
    //- Load widget's user attributes -
    tbl = owner().owner().tbl()+"_uio";
    c_el.setElem(&mod->elWdgUIO());
    c_el.cfg("IDW").setS(owner().id());
    c_el.cfg("ID").setS("");
    int fld_cnt = 0;
    while( SYS->db().at().dataSeek(db+"."+tbl,mod->nodePath()+tbl,fld_cnt++,c_el) )
    {
	string sid = c_el.cfg("ID").getS();
	unsigned flg = c_el.cfg("IO_TYPE").getI();
	c_el.cfg("ID").setS("");

	if( TSYS::pathLev(sid,0) == id() && !TSYS::pathLev(sid,1).empty() )
	    sid = TSYS::pathLev(sid,1);
	else continue;

	if( !attrPresent(sid) )
	    attrAdd( new TFld(sid.c_str(),c_el.cfg("NAME").getS().c_str(),(TFld::Type)(flg&0x0f),flg>>4) );
	AutoHD<Attr> attr = attrAt(sid);
	if( !(!(attr.at().flgGlob()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser) ) continue;
	attr.at().setS(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),0,'|'));
	attr.at().fld().setValues(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),1,'|'));
	attr.at().fld().setSelNames(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),2,'|'));
	attr.at().setFlgSelf((Attr::SelfAttrFlgs)c_el.cfg("SELF_FLG").getI());
	attr.at().setCfgTempl(c_el.cfg("CFG_TMPL").getS());
	attr.at().setCfgVal(c_el.cfg("CFG_VAL").getS());
    }
}

void CWidget::save_( )
{
    string db  = owner().owner().DB();
    string tbl = owner().owner().tbl();

    //- Delete from DB -
    if( nodeMode() == TCntrNode::Disable )
    {
	//-- Remove from library table --
	if( delMark )
	{
	    m_parent = "<deleted>";
	    SYS->db().at().dataSet( db+"."+tbl+"_incl", mod->nodePath()+tbl+"_incl", *this );
	}
	else SYS->db().at().dataDel( db+"."+tbl+"_incl", mod->nodePath()+tbl+"_incl", *this );	

	//-- Remove widget's work and users IO from library IO table --
	TConfig c_el( &mod->elWdgIO() );
	c_el.cfg("IDW").setS( owner().id() );
	int io_cnt = 0;
	while( SYS->db().at().dataSeek( db+"."+tbl+"_io", mod->nodePath()+tbl+"_io", io_cnt++, c_el ) )
	{
	    if( c_el.cfg("ID").getS().find(id()+"/") == 0 )
	    { SYS->db().at().dataDel( db+"."+tbl+"_io", mod->nodePath()+tbl+"_io", c_el ); io_cnt--; }
	    c_el.cfg("ID").setS("");
	}
	c_el.setElem(&mod->elWdgUIO());
	c_el.cfg("IDW").setS( owner().id() );
	io_cnt = 0;
	while( SYS->db().at().dataSeek( db+"."+tbl+"_uio", mod->nodePath()+tbl+"_uio", io_cnt++, c_el ) )
	{
	    if( c_el.cfg("ID").getS().find(id()+"/") == 0 )
	    { SYS->db().at().dataDel( db+"."+tbl+"_uio", mod->nodePath()+tbl+"_uio", c_el ); io_cnt--; }
	    c_el.cfg("ID").setS("");
	}
    }
    //- Save widget's data -
    else
    {
	//-- Save generic widget's data --
	m_attrs="";
	vector<string> als;
	attrList( als );
	for( int i_a = 0; i_a < als.size(); i_a++ )
	{
	    AutoHD<Attr> attr = attrAt(als[i_a]);
	    if( attr.at().modif() && !(!(attr.at().flgGlob()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser) )
		m_attrs+=als[i_a]+";";
	}
	SYS->db().at().dataSet(db+"."+tbl+"_incl",mod->nodePath()+tbl+"_incl",*this);

	//-- Save widget's attributes --
	saveIO();
    }
}

void CWidget::saveIO( )
{
    vector<string> als;

    //- Save widget's attributes -
    string db  = owner().owner().DB();
    string tbl = owner().owner().tbl()+"_io";
    string utbl = owner().owner().tbl()+"_uio";

    attrList( als );
    TConfig c_el(&mod->elWdgIO());
    c_el.cfg("IDW").setS(owner().id());
    TConfig c_elu(&mod->elWdgUIO());
    c_elu.cfg("IDW").setS(owner().id());
    for( int i_a = 0; i_a < als.size(); i_a++ )
    {
	AutoHD<Attr> attr = attrAt(als[i_a]);
	if( !attr.at().modif() )	continue;
	if( !(attr.at().flgGlob()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser )
	{
	    //-- User attribute store --
	    c_elu.cfg("ID").setS( id()+"/"+als[i_a] );
	    c_elu.cfg("IO_VAL").setS(attr.at().getS()+"|"+
				    attr.at().fld().values()+"|"+
				    attr.at().fld().selNames());
	    c_elu.cfg("NAME").setS(attr.at().name());
	    c_elu.cfg("IO_TYPE").setI(attr.at().fld().type()+(attr.at().fld().flg()<<4));
	    c_elu.cfg("SELF_FLG").setI(attr.at().flgSelf());
	    c_elu.cfg("CFG_TMPL").setS(attr.at().cfgTempl());
	    c_elu.cfg("CFG_VAL").setS(attr.at().cfgVal());
	    SYS->db().at().dataSet(db+"."+utbl,mod->nodePath()+utbl,c_elu);
	}
	else
	{
	    //-- Work attribute store --
	    c_el.cfg("ID").setS( id()+"/"+als[i_a] );
	    c_el.cfg("IO_VAL").setS(attr.at().getS());
	    c_el.cfg("SELF_FLG").setI(attr.at().flgSelf());
	    c_el.cfg("CFG_TMPL").setS(attr.at().cfgTempl());
	    c_el.cfg("CFG_VAL").setS(attr.at().cfgVal());
	    SYS->db().at().dataSet(db+"."+tbl,mod->nodePath()+tbl,c_el);
	}
    }
    //- Clear no present IO for user io table -
    int fld_cnt=0;
    c_elu.cfg("ID").setS("");
    c_elu.cfgViewAll(false);
    while( SYS->db().at().dataSeek(db+"."+utbl,mod->nodePath()+utbl,fld_cnt++,c_elu) )
    {
	string sid = c_elu.cfg("ID").getS();
	if( TSYS::pathLev(sid,0) == id() && TSYS::pathLev(sid,1).size() && !attrPresent(TSYS::pathLev(sid,1)) )
	{
	    SYS->db().at().dataDel(db+"."+utbl,mod->nodePath()+utbl,c_elu);
	    fld_cnt--;
	}
	c_elu.cfg("ID").setS("");
    }
}

string CWidget::resourceGet( const string &id, string *mime )
{
    string mimeType, mimeData;

    if( (mimeData=owner().resourceGet( id, &mimeType )).empty() && !parent().freeStat() )
	mimeData = parent().at().resourceGet( id, &mimeType );
    if( mime )	*mime = mimeType;

    return mimeData;
}

void CWidget::inheritAttr( const string &attr )
{
    bool mdf = isModify();
    Widget::inheritAttr( attr );
    if( !mdf )  modifClr( );
}

void CWidget::cntrCmdProc( XMLNode *opt )
{
    if( cntrCmdServ(opt) ) return;
    //- Get page info -
    if( opt->name() == "info" )
    {
	cntrCmdGeneric(opt);
	cntrCmdAttributes(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Link to widget: ")+id(),permit(),user().c_str(),grp().c_str());
	return;
    }
    if( !(cntrCmdGeneric(opt) || cntrCmdAttributes(opt)) )
	TCntrNode::cntrCmdProc(opt);
}
