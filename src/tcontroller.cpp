
#include "tbds.h"
#include "tmessage.h"
#include "tcontrollers.h"
#include "ttipcontroller.h"
#include "tparamcontr.h"
#include "tcontroller.h"

const char *TController::o_name = "TController";

//==== TController ====
 TController::TController( TTipController *tcntr, string name_c, string _t_bd, string _n_bd, string _n_tb, TConfigElem *cfgelem) : 
		name(name_c), t_bd(_t_bd), n_bd(_n_bd), n_tb(_n_tb), owner(tcntr), TConfig(cfgelem), stat(TCNTR_DISABLE)
{
    Set_S("NAME",name_c);    
}

 TController::~TController(  )
{
    while(cntr_prm.size())
    {
	delete cntr_prm[0];
	cntr_prm.erase(cntr_prm.begin());
    }
}


void TController::Load( )
{
    TBDS *bds = owner->owner->owner->BD;
    if( stat == TCNTR_ENABLE || stat == TCNTR_RUN ) 
    {
	Set_S("NAME",name);
	int i_hd = bds->OpenTable(t_bd,n_bd,n_tb);
	LoadValBD("NAME",bds->at_tbl(i_hd));
	bds->CloseTable(i_hd);	

	LoadParmCfg( );
#if OSC_DEBUG
    	Mess->put(1, "Load controller's configs: <%s>!",Name().c_str());
#endif   
    }
    else throw TError("%s: Controller %s no enable!",o_name,Name().c_str());
}

void TController::Save( )
{
    int i_hd;
    TBDS *bds = owner->owner->owner->BD;
    if( stat == TCNTR_ENABLE || stat == TCNTR_RUN) 
    {
	SaveParmCfg( );
	
	try{ i_hd = bds->OpenTable(t_bd,n_bd,n_tb); }
	catch(...){ i_hd = bds->OpenTable(t_bd,n_bd,n_tb,true); }	
	owner->ConfigElem()->UpdateBDAttr( bds->at_tbl(i_hd) );
	SaveValBD("NAME",bds->at_tbl(i_hd));
	bds->CloseTable(i_hd);
#if OSC_DEBUG
	Mess->put(1, "Save controller's configs: <%s>!",Name().c_str());	
#endif 
    }
    else throw TError("%s: Controller %s no enable!",o_name,Name().c_str());
} 

void TController::Free(  )
{
    if( stat == TCNTR_ENABLE || stat == TCNTR_RUN) 
    {
	FreeParmCfg( );
#if OSC_DEBUG
	Mess->put(1, "Free controller's configs: <%s>!",Name().c_str());
#endif 
    }
    else throw TError("%s: Controller %s no enable!",o_name,Name().c_str());
}    

void TController::Start( )
{
    if( stat == TCNTR_ENABLE )
    {
	//Set valid all parameter
	for(unsigned i_p=0; i_p < cntr_prm.size(); i_p++)
	    cntr_prm[i_p]->Enable();

	stat = TCNTR_RUN;
#if OSC_DEBUG
	Mess->put(1, "Start controller: <%s>!",Name().c_str());
#endif 	
    }
    else if( stat == TCNTR_RUN ) return;
    else throw TError("%s: Controller %s no enable!",o_name,Name().c_str());
}

void TController::Stop( )
{
    if( stat == TCNTR_RUN )
    {
    	stat = TCNTR_ENABLE;
	//Set valid all parameter
	for(unsigned i_p=0; i_p < cntr_prm.size(); i_p++)
	    cntr_prm[i_p]->Disable();	
#if OSC_DEBUG
	Mess->put(1, "Stop controller: <%s>!",Name().c_str());
#endif	
    }
    else throw TError("%s: Controller %s no run!",o_name,Name().c_str());
}

void TController::Enable( )
{
    if( stat == TCNTR_DISABLE )
    {
	stat = TCNTR_ENABLE;
    	Load( );
	RegParamS();
#if OSC_DEBUG
	Mess->put(1, "Enable controller: <%s>!",Name().c_str());
#endif
    }
}

void TController::Disable( )
{
    if( stat == TCNTR_ENABLE || stat == TCNTR_RUN )
    {
	if( stat == TCNTR_RUN ) Stop( );
	Free( );
	stat = TCNTR_DISABLE;
#if OSC_DEBUG
	Mess->put(1, "Disable controller: <%s>!",Name().c_str());
#endif
    }
}

void TController::LoadParmCfg(  )
{
    int         t_hd;
    string      parm_bd;
    TParamContr *PrmCntr;

    TBDS    *bds  = owner->owner->owner->BD;    
    TParamS *prms = owner->owner->owner->Param;    
    
    time_t tm = time(NULL);
    for(unsigned i_tp = 0; i_tp < owner->SizeTpPrm(); i_tp++)
    {
	t_hd = bds->OpenTable(t_bd,n_bd,Get_S(owner->at_TpPrm(i_tp)->BD()));
	for(unsigned i=0; i < (unsigned)bds->at_tbl(t_hd)->NLines( ); i++)
	{
	    //Load param config fromBD
	    PrmCntr = ParamAttach(i_tp);
	    PrmCntr->LoadValBD(i,bds->at_tbl(t_hd));
    	    PrmCntr->UpdateVAL( );    
    	    PrmCntr->t_sync=tm;
	    //!!! Want request resource
	    //Find already loading param
	    unsigned i_prm;
	    for(i_prm=0; i_prm < cntr_prm.size(); i_prm++)
		if(*PrmCntr == *cntr_prm[i_prm]) break;
	    if( i_prm == cntr_prm.size())
	    {
		cntr_prm.push_back(PrmCntr);
		HdIns(i_prm);
	    }
	    else
	    {
		*cntr_prm[i_prm] = *PrmCntr;
		delete PrmCntr;
	    }
	}
    }
    bds->CloseTable(t_hd);
    //Check freeing param
    for(unsigned i_prm=0; i_prm < cntr_prm.size(); i_prm++)
	if( tm != cntr_prm[i_prm]->t_sync )
	{
	    prms->Del(cntr_prm[i_prm]);
	    HdFree(i_prm);		
	    delete cntr_prm[i_prm];
	    cntr_prm.erase(cntr_prm.begin()+i_prm);
	    i_prm--;
	}
	//!!! Want free resource
}

TParamContr *TController::ParamAttach(int type)
{
    return(new TParamContr(this, owner->at_TpPrm(type)));
}

void TController::SaveParmCfg(  )
{
    int    t_hd;

    TBDS    *bds  = owner->owner->owner->BD;    
    for(unsigned i_tp = 0; i_tp < owner->SizeTpPrm(); i_tp++)
    {
    	string parm_tbl = Get_S(owner->at_TpPrm(i_tp)->BD());
    
	//Update BD (resize, change atributes ..
	try{ t_hd = bds->OpenTable(t_bd,n_bd,parm_tbl); }
	catch(...){ t_hd = bds->OpenTable(t_bd,n_bd,parm_tbl,true); }    
    	owner->at_TpPrm(i_tp)->UpdateBDAttr(bds->at_tbl(t_hd));
	//Clear BD
	while(bds->at_tbl(t_hd)->NLines( )) bds->at_tbl(t_hd)->DelLine(0);
	time_t tm = time(NULL);
	for(unsigned i_ln=0; i_ln < cntr_prm.size(); i_ln++)
	    if(cntr_prm[i_ln]->Type()->Name() == owner->at_TpPrm(i_tp)->Name())
	    {			
		bds->at_tbl(t_hd)->AddLine(i_ln);
		cntr_prm[i_ln]->SaveValBD(i_ln,bds->at_tbl(t_hd));
		cntr_prm[i_ln]->t_sync=tm;
	    }
	bds->at_tbl(t_hd)->Save( );
	bds->CloseTable(t_hd);
    }
}

void TController::FreeParmCfg(  )
{
    //!!! Want request resource
    while(cntr_prm.size())
    {
	owner->owner->owner->Param->Del(cntr_prm[0]);
	HdFree(0);		
	delete cntr_prm[0];
	cntr_prm.erase(cntr_prm.begin());
    }
    //!!! Want free resource
}

void TController::RegParam( unsigned id_hd )
{
    if(id_hd >= hd.size() || hd[id_hd] < 0 ) 
    	throw TError("%s: header %d error!",o_name,id_hd);
    owner->owner->owner->Param->Add(cntr_prm[hd[id_hd]]); 
}

void TController::RegParamS()
{
    for(unsigned i_prm = 0; i_prm < cntr_prm.size(); i_prm++)
	owner->owner->owner->Param->Add(cntr_prm[i_prm]);
}

void TController::UnRegParam( unsigned id_hd )
{ 
    if(id_hd >= hd.size() || hd[id_hd] < 0 ) 
    	throw TError("%s: header %d error!",o_name,id_hd);
    owner->owner->owner->Param->Del(cntr_prm[hd[id_hd]]);
}

void TController::UnRegParamS()
{
    for(unsigned i_prm = 0; i_prm < cntr_prm.size(); i_prm++)
	owner->owner->owner->Param->Del(cntr_prm[i_prm]);
}

void TController::List( vector<string> & List )
{
    List.clear();
    if( stat == TCNTR_DISABLE ) throw TError("%s: %s controller disabled!",o_name,Name().c_str());
    for(unsigned i_prmc=0; i_prmc < cntr_prm.size(); i_prmc++)
	List.push_back(cntr_prm[i_prmc]->Name());
}


unsigned TController::Add( string Name_TP, string name, int pos )
{
    unsigned i_prmc;
    
    TParamContr *PrmCntr;
    if( stat == TCNTR_DISABLE ) throw TError("%s: %s controller disabled!",o_name,Name().c_str());
    
    //!!! Want request resource
    //Find param name
    for(i_prmc=0; i_prmc < cntr_prm.size(); i_prmc++)
	if(cntr_prm[i_prmc]->Name() == name ) 
	{
	    //!!! Want free resource    		
	    throw TError("%s: Parameter %s already avoid!",o_name,name.c_str());
	}
    
    PrmCntr = ParamAttach( owner->NameTpPrmToId(Name_TP) );
    PrmCntr->Set_S("SHIFR",name);  PrmCntr->t_sync = time(NULL);
    if(pos < 0 || pos >= (int)cntr_prm.size() ) 
	pos = (int)cntr_prm.size();
    cntr_prm.insert(cntr_prm.begin() + pos,PrmCntr);
    HdIns(pos);
    owner->owner->owner->Param->Add(cntr_prm[pos]);
    //!!! Want free resource    
    return((unsigned)pos);
}

void TController::Del( string name )
{
    if( stat == TCNTR_DISABLE ) throw TError("%s: %s controller disabled!",o_name,Name().c_str());

    //!!! Want request resource
    for(unsigned i_prmc=0; i_prmc < cntr_prm.size(); i_prmc++)
	if(cntr_prm[i_prmc]->Name() == name )
	{    
	    owner->owner->owner->Param->Del(cntr_prm[i_prmc]);
	    delete cntr_prm[i_prmc];
	    cntr_prm.erase(cntr_prm.begin() + i_prmc);
	    HdFree(i_prmc);		
	    //!!! Want free resource    	    
	    return;
	}
    //!!! Want free resource    	    
    throw TError("%s: %s parameter no avoid!",o_name,name.c_str());
}
    
void TController::Rotate( string name1, string name2)
{
    int id1= -1,id2= -1;
	
    if( stat == TCNTR_DISABLE ) throw TError("%s: %s controller disabled!",o_name,Name().c_str());

    //!!! Want request resource
    for(unsigned i_prmc=0; i_prmc < cntr_prm.size(); i_prmc++)
	if(cntr_prm[i_prmc]->Name() == name1 ) { id1 = i_prmc; break; }
    for(unsigned i_prmc=0; i_prmc < cntr_prm.size(); i_prmc++)
	if(cntr_prm[i_prmc]->Name() == name2 ) { id2 = i_prmc; break; }
    if(id1 < 0 || id2 < 0 || id1 == id2) 
    {
	//!!! Want free resource
       	throw TError("%s: error parameters!",o_name);
    }
    TParamContr *PrmCntr = cntr_prm[id1];    
    cntr_prm[id1]   = cntr_prm[id2];    
    cntr_prm[id2]   = PrmCntr;
    HdChange(id1,id2);
    //!!! Want free resource
}

unsigned TController::HdIns( unsigned id )
{
    unsigned i_hd;
    for( i_hd=0; i_hd < hd.size(); i_hd++)
	if( hd[i_hd] >= (int)id ) hd[i_hd]++;
    for( i_hd=0; i_hd < hd.size(); i_hd++)
	if(hd[i_hd] < 0 ) break;
    if( i_hd == hd.size() ) hd.push_back();
    hd[i_hd] = id; 

    return(i_hd);    
}

void TController::HdFree( unsigned id )
{
    for(unsigned i_hd=0; i_hd < hd.size(); i_hd++)
	if( hd[i_hd] == (int)id ) { hd[i_hd] = -1; break; } 
    for(unsigned i_hd=0; i_hd < hd.size(); i_hd++)
	if( hd[i_hd] > (int)id ) hd[i_hd]--;
} 

void TController::HdChange( unsigned id1, unsigned id2 )
{
    for(unsigned i_hd = 0; i_hd < hd.size(); i_hd++)
	if( hd[i_hd] == (int)id1 )      { hd[i_hd] = id2; continue; }
	else if( hd[i_hd] == (int)id2 ) { hd[i_hd] = id1; continue; }
}


unsigned TController::NameToHd( string Name )
{
    for(unsigned i_hd = 0; i_hd < hd.size(); i_hd++)
	if(hd[i_hd] >= 0 && cntr_prm[hd[i_hd]]->Name() == Name ) return(i_hd);
    throw TError("%s: parameter %s no avoid!",o_name,Name.c_str());
}

TParamContr *TController::at( unsigned id_hd )    
{
    if(id_hd >= hd.size() || hd[id_hd] < 0) 
       	throw TError("%s: parameter's header error %d!",o_name,id_hd); 
    return(cntr_prm[hd[id_hd]]);    
}


