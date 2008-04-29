
//OpenSCADA system module Transport.Sockets file: socket.h
/***************************************************************************
 *   Copyright (C) 2003-2007 by Roman Savochenko                           *
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
 
#ifndef SOCKET_H
#define SOCKET_H

#include <pthread.h>

#include <ttransports.h>

#undef _
#define _(mess) mod->I18N(mess)

#define S_NM_TCP  "TCP"
#define S_NM_UDP  "UDP"
#define S_NM_UNIX "UNIX"

#define SOCK_TCP  0
#define SOCK_UDP  1
#define SOCK_UNIX 2

namespace Sockets
{

class TSocketIn;

//************************************************
//* Sockets::SSockIn                             *
//************************************************
class SSockIn 
{
    public:
	SSockIn( TSocketIn *is_in, int icl_sock, string isender ) :
	    s_in(is_in), cl_sock(icl_sock), sender(isender)	{  }
	    
	TSocketIn *s_in;
	int       cl_sock;
	string    sender;
};

//************************************************
//* Sockets::SSockCl                             *
//************************************************
struct SSockCl
{
    pid_t  cl_id;    // Client's pids
    int    cl_sock;
};

//************************************************
//* Sockets::TSocketIn                           *
//************************************************
class TSocketIn: public TTransportIn
{
    public:
	/* Open input socket <name> for locale <address>
	 * address : <type:<specific>>
	 * type: 
	 *   TCP  - TCP socket with  "UDP:<host>:<port>"
	 *   UDP  - UDP socket with  "TCP:<host>:<port>"
	 *   UNIX - UNIX socket with "UNIX:<path>"
	 */
	TSocketIn(string name,const string &idb,TElem *el);
	~TSocketIn( );
	
	int maxQueue( )		{ return max_queue; }
	int maxFork( )		{ return max_fork; }
	int bufLen( )		{ return buf_len; }
	
	void setMaxQueue( int vl )	{ max_queue = vl; modif(); }
	void setMaxFork( int vl )	{ max_fork = vl; modif(); }
	void setBufLen( int vl )	{ buf_len = vl; modif(); }
	
	void start( );
	void stop( );
	    
    private:
	//Methods
	static void *Task(void *);
	static void *ClTask(void *);
	
	void ClSock( SSockIn &s_in );
	void PutMess( int sock, string &request, string &answer, string sender, AutoHD<TProtocolIn> &prot_in );

	void RegClient(pthread_t thrid, int i_sock);
	void UnregClient(pthread_t thrid);
	    
	void cntrCmdProc( XMLNode *opt );       //Control interface command process
	    
	//Attributes
	pthread_t pthr_tsk;
	int       sock_fd;
	Res       sock_res;
	
	bool      endrun;      // Command for stop task	    
	bool      endrun_cl;   // Command for stop client tasks

	int       &max_queue;   // max queue for TCP, UNIX sockets
	int       &max_fork;    // maximum forking (opened sockets)
	int       &buf_len;     // input buffer length	

	int       type;        // socket's types 
	string    path;        // path to file socket for UNIX socket
	string    host;        // host for TCP/UDP sockets
	string    port;        // port for TCP/UDP sockets
	int       mode;        // mode for TCP/UNIX sockets (0 - no hand; 1 - hand connect)

	bool            cl_free;  // Clients stoped
	vector<SSockCl> cl_id;    // Client's pids
};

//************************************************
//* Sockets::TSocketOut                          *
//************************************************
class TSocketOut: public TTransportOut
{
    public:
	/* Open output socket <name> for locale <address>
	 * address : <type:<specific>>
	 * type: 
	 *   TCP  - TCP socket with  "UDP:<host>:<port>"
	 *   UDP  - UDP socket with  "TCP:<host>:<port>"
	 *   UNIX - UNIX socket with "UNIX:<path>"
	 */
	TSocketOut(string name,const string &idb,TElem *el);
	~TSocketOut();

	void start();
	void stop();

	int messIO( const char *obuf, int len_ob, char *ibuf = NULL, int len_ib = 0, int time = 0 );

    private:
	int	sock_fd;
    
	int	type;        // socket's types 
	struct sockaddr_in  name_in;
	struct sockaddr_un  name_un;
};

//************************************************
//* Sockets::TTransSock                          *
//************************************************
class TTransSock: public TTipTransport
{
    public:
	TTransSock( string name );
	~TTransSock( );
	    
	TTransportIn  *In( const string &name, const string &idb );
	TTransportOut *Out( const string &name, const string &idb );
	
    protected:
	void load_( );

    private:	
	//Methods
	string optDescr( );
	void cntrCmdProc( XMLNode *opt );       //Control interface command process
	    
	void postEnable( int flag );
	    
	//Attributes
	static const char *i_cntr; 
};
    
extern TTransSock *mod;    
}

#endif //SOCKET_H

