#include "cbase.h"
#include "dbhandler.h"
#include "tier0/icommandline.h"
#include "filesystem.h"

dbHandler g_pDB;
dbHandler *TheDatabase = &g_pDB;

void DebugDatabase_ChangeCallback( IConVar *pConVar, char const *pOldString, float flOldValue );
ConVar debug_database( "debug_database", "0", FCVAR_CHEAT, "Show all database commands in the server console", true, 0, true, 1, DebugDatabase_ChangeCallback );
void DebugDatabase_ChangeCallback( IConVar *pConVar, char const *pOldString, float flOldValue )
{
	TheDatabase->EnableDebugging( debug_database.GetInt() == 1 );
}

#ifndef CLIENT_DLL
#define VarArgs UTIL_VarArgs
#endif

dbHandler::dbHandler()
{
	m_bIsDebugging = false;
	m_iInTransaction = 0;
}

dbHandler::~dbHandler()
{
	sqlite3_close(db);
	if ( m_bIsDebugging )
		Msg("database connection closed\n");
}

// returns true if the database already existed, false otherwise (it will be created in this case)
void dbHandler::Initialise(const char *filename)
{
	int rc = sqlite3_open(VarArgs("%s\\%s",CommandLine()->ParmValue( "-game", "hl2" ),filename), Reference());

	if( rc && m_bIsDebugging )
		Msg("Failed to open database: %s\n", sqlite3_errmsg(Instance()));
}

void dbHandler::ShowError(int returnCode, const char *action, const char *command, const char *customError)
{
	Warning(VarArgs("Database error #%i when %s command:\n%s\nError message: %s\n", returnCode, action, command, customError == NULL ? sqlite3_errmsg(db) : customError));
}

void dbHandler::BeginTransaction()
{
	m_iInTransaction ++;
	if ( m_iInTransaction == 1 )
	{
		bool bDebugging = m_bIsDebugging;
		m_bIsDebugging = false; // don't write out BEGIN / COMMIT commands
		Command("BEGIN");
		m_bIsDebugging = bDebugging;
	}
}

void dbHandler::CommitTransaction()
{
	m_iInTransaction = MIN(m_iInTransaction-1,0);
	if ( m_iInTransaction == 0 )
	{
		bool bDebugging = m_bIsDebugging;
		m_bIsDebugging = false; // don't write out BEGIN / COMMIT commands
		Command("COMMIT");
		m_bIsDebugging = bDebugging;
	}
}

// execute a command that outputs no data, returns true on success or false on failure
bool dbHandler::Command(const char *cmd, ...)
{
	va_list marker;
	char msg[4096];
	va_start(marker, cmd);
	Q_vsnprintf(msg, sizeof(msg), cmd, marker);
	va_end(marker);
	const char *command = VarArgs( msg );
	
	if ( m_bIsDebugging )
		Msg("%s\n",command);

	bool retVal = true;
	sqlite3_stmt *stmt;
	int rc = sqlite3_prepare_v2(db, command, -1, &stmt, 0);
	if( rc )
	{
		ShowError(rc, "preparing", command);
		retVal = false;
	}
	else
	{
		rc = sqlite3_step(stmt);
		switch( rc )
		{
			case SQLITE_DONE:
			case SQLITE_OK:
				break;
			default:
				ShowError(rc, "processing", command);
				retVal = false;
				break;
		}
		
		// finalize the statement to release resources
		rc = sqlite3_finalize(stmt);
		if( rc != SQLITE_OK)
			ShowError(rc, "finalising", command);
	}
	
	return retVal;
}

const char* dbHandler::ReadString(const char *cmd, ...)
{
	va_list marker;
	char msg[4096];
	va_start(marker, cmd);
	Q_vsnprintf(msg, sizeof(msg), cmd, marker);
	va_end(marker);
	const char *command = VarArgs( msg );

	if ( m_bIsDebugging )
		Msg("%s\n",command);

	bool isnull = true;
	char retVal[MAX_DB_STRING];

	sqlite3_stmt *stmt;
	int rc = sqlite3_prepare_v2(db, command, -1, &stmt, 0);
	if( rc )
		ShowError(rc, "preparing", command);
	else
	{
		//int cols = sqlite3_column_count(stmt);
		do
		{
			rc = sqlite3_step(stmt);
			switch( rc )
			{
				case SQLITE_DONE:
				case SQLITE_OK:
					break;
				case SQLITE_ROW:
					// print results for this row, the only row (hopefully)
					Q_snprintf(retVal, sizeof(retVal), (const char*)sqlite3_column_text(stmt, 0));
					isnull = false;
					break;
				default:
					ShowError(rc, "processing", command);
					break;
			}
		} while( rc==SQLITE_ROW );
		// finalize the statement to release resources
		rc = sqlite3_finalize(stmt);
		if( rc != SQLITE_OK)
			ShowError(rc, "finalising", command);
	}
	
	if ( m_bIsDebugging )
		Msg("returning: %s\n", isnull ? "null" : retVal);
	return isnull ? NULL : retVal;
}

int dbHandler::ReadInt(const char *cmd, ...)
{
	va_list marker;
	char msg[4096];
	va_start(marker, cmd);
	Q_vsnprintf(msg, sizeof(msg), cmd, marker);
	va_end(marker);
	const char *command = VarArgs( msg );
	
	if ( m_bIsDebugging )
		Msg("%s\n",command);

	int retVal = -1;
	sqlite3_stmt *stmt;
	int rc = sqlite3_prepare_v2(db, command, -1, &stmt, 0);
	if( rc != SQLITE_OK)
		ShowError(rc, "preparing", command);
	else
	{
		//int cols = sqlite3_column_count(stmt);
		do
		{
			rc = sqlite3_step(stmt);
			switch( rc )
			{
				case SQLITE_DONE:
				case SQLITE_OK:
					break;
				case SQLITE_ROW:
					// print results for this row, the only row (hopefully)
					//retVal = sqlite3_column_int64(stmt, 0); - primary key values are int64, I think?
					retVal = sqlite3_column_int(stmt, 0);
					break;
				default:
					ShowError(rc, "processing", command);
					break;
			}
		} while( rc==SQLITE_ROW );
		// finalize the statement to release resources
		rc = sqlite3_finalize(stmt);
		if( rc != SQLITE_OK)
			ShowError(rc, "finalising", command);
	}
	
	if ( m_bIsDebugging )
		Msg(VarArgs("returning: %i\n",retVal));
	return retVal;
}

dbReadResult* dbHandler::ReadMultiple(const char *cmd, ...)
{
	va_list marker;
	char msg[4096];
	va_start(marker, cmd);
	Q_vsnprintf(msg, sizeof(msg), cmd, marker);
	va_end(marker);
	const char *command = VarArgs( msg );
	
	if ( m_bIsDebugging )
		Msg("%s\n",command);

	dbReadResult* output = new dbReadResult();
	sqlite3_stmt *stmt;
	int rc = sqlite3_prepare_v2(db, command, -1, &stmt, 0);
	if( rc )
	{
		ShowError(rc, "preparing", command);
	}
	else
	{
		int cols = sqlite3_column_count(stmt);
		// execute the statement
		do
		{
			rc = sqlite3_step(stmt);
			switch( rc )
			{
				case SQLITE_DONE:
				case SQLITE_OK:
					break;
				case SQLITE_ROW:
					for( int col=0; col<cols; col++)
						switch ( sqlite3_column_type(stmt, col) )
						{
							case SQLITE_INTEGER:
								output->AddToTail(dbValue(sqlite3_column_int(stmt, col))); break;
							case SQLITE_TEXT:
								output->AddToTail(dbValue((const char*)sqlite3_column_text(stmt, col))); break;
							case SQLITE_FLOAT:
								output->AddToTail(dbValue(sqlite3_column_double(stmt, col))); break;
                            case SQLITE_NULL:
                                output->AddToTail( dbValue( 0 ) ); break;
							case SQLITE_BLOB:
							default:
								ShowError(rc, "processing", command, "Data type is not supported; must be SQLITE_INTEGER, SQLITE_FLOAT or SQLITE_TEXT!"); break;
						}
					break;
				default:
					ShowError(rc, "processing", command);
					break;
			}
		} while( rc==SQLITE_ROW );
		// finalize the statement to release resources
		rc = sqlite3_finalize(stmt);
		if( rc != SQLITE_OK)
			ShowError(rc, "finalising", command);
	}

	if ( m_bIsDebugging )
	{
		Msg("returning: ");
		int num = output->Count();
		for (int i=0; i<num; i++ )
		{
			if ( i > 0 )
				Msg(", ");
			switch ( output->Element(i).type )
			{
			case INTEGER:
				Msg(VarArgs("%i",output->Element(i).integer)); break;
			case TEXT:
				Msg(VarArgs("%s",output->Element(i).text)); break;
			case FLOATING:
				Msg(VarArgs("%f",output->Element(i).floating)); break;
			default:
				Msg("<INVALID>"); break;
			}
		}
		Msg("\n");
	}
	return output;
}

int dbHandler::LastInsertID()
{
	return sqlite3_last_insert_rowid(db);
}