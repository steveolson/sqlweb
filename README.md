SQLweb -- the SQL Interface to the WEB ... SQLweb ...

   Copyright (c) 1995-1999 Applied Information Technologies, Inc.
   All Rights Reserved.
    
   Distributed uder the GNU General Public License which was included in
   the file named "LICENSE" in the package that you recieved.
   If not, write to:
   The Free Software Foundation, Inc.,
   675 Mass Ave, Cambridge, MA 02139, USA.
    
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See 
   the LICENSE for more details.

---- Installation ----

I. Install and create your database 
    1. Follow database install instructions for Oracle, Informix, or mSQL

II. Build SQLweb core libraries

    1. Build lib/liblist.a
       1.1 (cd list; make install)
    2. Build lib/libSQLweb.a
       2.1 (cd libSQLweb; make install)

III. Continue with SQLweb "binary installation"

    1. Build SQLweb executable
       1.1 cd <your-database-dir>
	   1.1.1 Oracle; cd dbd/oracle
	   1.1.2 Informix; cd dbd/informix
	   1.1.3 mSQL; cd dbd/msql
       1.2 Modify the Makefile to suite your needs, verify 
	   LDFLAGS for proper libraries.  Check any special needs
	   required for your database.
       1.3 make sqlweb

    2. Modify the ./sqlweb.ini file
	2.1 Modify the following variables in the [Env] Section
	    2.1.1 Oracle
	    ORACLE_HOME
	    ORACLE_SID

	    2.1.2 INFORMIX
	    INFORMIXDIR
	    INFORMIXSQLHOSTS
	    INFORMIXSERVER
	    ONCONFIG

	    2.1.3 mSQL -- none

	2.2 Modify the following variables in the [Symbol] Section
	    2.2.1 Oracle
	    ORACLE_CONNECT

	    2.2.2 Informix
	    INFORMIX_DRIVERCONNECT

	    2.2.3 mSQL
	    MSQL_DB

    3. Install the SQLweb executable and sqlweb.ini file into the
    desired directory.

	1. cp sqlweb.ini <your-favorite-dir>
	   cd oracle; cp sqlweb <your-favorite-dir>


That's all, for more information see http://www.sqlweb.com/doc
