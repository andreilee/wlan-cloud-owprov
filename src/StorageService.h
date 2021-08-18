//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#ifndef UCENTRAL_USTORAGESERVICE_H
#define UCENTRAL_USTORAGESERVICE_H

#include "Poco/Data/Session.h"
#include "Poco/Data/SessionPool.h"
#include "Poco/Data/SQLite/Connector.h"

#include "Poco/Data/PostgreSQL/Connector.h"
#include "Poco/Data/MySQL/Connector.h"
#include "SubSystemServer.h"

#include "orm.h"

#include "storage_entity.h"
#include "storage_policies.h"
#include "storage_venue.h"
#include "storage_location.h"
#include "storage_contact.h"
#include "storage_inventory.h"

namespace OpenWifi {

    class Storage : public uCentral::SubSystemServer {
    public:
        static Storage *instance() {
            if (instance_ == nullptr) {
                instance_ = new Storage;
            }
            return instance_;
        }

		int 	Start() override;
		void 	Stop() override;

		[[nodiscard]] std::string ConvertParams(const std::string &S) const;

		int 	Setup_SQLite();
		int 	Setup_MySQL();
		int 	Setup_PostgreSQL();

		OpenWifi::EntityDB & EntityDB() { return *EntityDB_; };
		OpenWifi::PolicyDB & PolicyDB() { return *PolicyDB_; };
		OpenWifi::VenueDB & VenueDB() { return *VenueDB_; };
		OpenWifi::LocationDB & LocationDB() { return *LocationDB_; };
		OpenWifi::ContactDB & ContactDB() { return *ContactDB_;};
		OpenWifi::InventoryDB & InventoryDB() { return *InventoryDB_; };


	  private:
		static Storage      								*instance_;
		std::unique_ptr<Poco::Data::SessionPool>        	Pool_= nullptr;
		std::unique_ptr<Poco::Data::SQLite::Connector>  	SQLiteConn_= nullptr;
		std::unique_ptr<Poco::Data::PostgreSQL::Connector>  PostgresConn_= nullptr;
		std::unique_ptr<Poco::Data::MySQL::Connector>       MySQLConn_= nullptr;

		ORM::DBType                                         DBType_ = ORM::DBType::sqlite;
		std::unique_ptr<OpenWifi::EntityDB>                 EntityDB_;
		std::unique_ptr<OpenWifi::PolicyDB>                 PolicyDB_;
		std::unique_ptr<OpenWifi::VenueDB>                  VenueDB_;
		std::unique_ptr<OpenWifi::LocationDB>               LocationDB_;
		std::unique_ptr<OpenWifi::ContactDB>                ContactDB_;
		std::unique_ptr<OpenWifi::InventoryDB>              InventoryDB_;

		Storage() noexcept;
   };

   inline Storage * Storage() { return Storage::instance(); }

}  // namespace

#endif //UCENTRAL_USTORAGESERVICE_H
