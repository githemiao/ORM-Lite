
#ifndef SQLITE_CONNECTOR_H
#define SQLITE_CONNECTOR_H

#include <string>

#include "sqlite3.h"
#include "ormlite.h"

namespace BOT_ORM_Impl
{
    class SqliteConnector : public SQLConnector
    {
    public:
        SqliteConnector (const std::string &fileName)
        {
            if (sqlite3_open (fileName.c_str (), &db) != SQLITE_OK)
                throw std::runtime_error (
                    std::string ("SQL error: Can't open database '")
                    + sqlite3_errmsg (db) + "'");
        }

        ~SqliteConnector ()
        {
            sqlite3_close (db);
        }

        void Execute (const std::string &cmd)
        {
            char *zErrMsg = 0;
            int rc = SQLITE_OK;

            for (size_t iTry = 0; iTry < MAX_TRIAL; iTry++)
            {
                rc = sqlite3_exec (db, cmd.c_str (), 0, 0, &zErrMsg);
                if (rc != SQLITE_BUSY)
                    break;

                std::this_thread::sleep_for (
                    std::chrono::microseconds (20));
            }

            if (rc != SQLITE_OK)
            {
                auto errStr = std::string ("SQL error: '") + zErrMsg
                    + "' at '" + cmd + "'";
                sqlite3_free (zErrMsg);
                throw std::runtime_error (errStr);
            }
        }

        void ExecuteCallback (const std::string &cmd,
            std::function<void (int, char **)>
            callback)
        {
            char *zErrMsg = 0;
            int rc = SQLITE_OK;

            auto callbackParam = std::make_pair (&callback, std::string {});

            for (size_t iTry = 0; iTry < MAX_TRIAL; iTry++)
            {
                rc = sqlite3_exec (db, cmd.c_str (), CallbackWrapper,
                    &callbackParam, &zErrMsg);
                if (rc != SQLITE_BUSY)
                    break;

                std::this_thread::sleep_for (
                    std::chrono::microseconds (20));
            }

            if (rc == SQLITE_ABORT)
            {
                auto errStr = "SQL error: '" + callbackParam.second +
                    "' at '" + cmd + "'";
                sqlite3_free (zErrMsg);
                throw std::runtime_error (errStr);
            }
            else if (rc != SQLITE_OK)
            {
                auto errStr = std::string ("SQL error: '") + zErrMsg
                    + "' at '" + cmd + "'";
                sqlite3_free (zErrMsg);
                throw std::runtime_error (errStr);
            }
        }

    private:
        sqlite3 *db;
        constexpr static size_t MAX_TRIAL = 16;

        static int CallbackWrapper (
            void *callbackParam, int argc, char **argv, char **)
        {
            auto pParam = static_cast<std::pair<
                std::function<void (int, char **)> *,
                std::string
            >*>(callbackParam);

            try
            {
                pParam->first->operator()(argc, argv);
                return 0;
            }
            catch (const std::exception &ex)
            {
                pParam->second = ex.what ();
                return 1;
            }
        }
    };
}

#endif