//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//


#ifndef OWPROV_RESTAPI_INVENTORY_HANDLER_H
#define OWPROV_RESTAPI_INVENTORY_HANDLER_H

#include "framework/MicroService.h"
#include "StorageService.h"

namespace OpenWifi {
    class RESTAPI_inventory_handler : public RESTAPIHandler {
    public:
        RESTAPI_inventory_handler(const RESTAPIHandler::BindingMap &bindings, Poco::Logger &L, RESTAPI_GenericServer & Server, bool Internal)
        : RESTAPIHandler(bindings, L,
                         std::vector<std::string>{
            Poco::Net::HTTPRequest::HTTP_GET, Poco::Net::HTTPRequest::HTTP_POST,
            Poco::Net::HTTPRequest::HTTP_PUT, Poco::Net::HTTPRequest::HTTP_DELETE,
            Poco::Net::HTTPRequest::HTTP_OPTIONS},
            Server,
            Internal),
            DB_(StorageService()->InventoryDB()){}
        static const std::list<const char *> PathName() { return std::list<const char *>{"/api/v1/inventory/{serialNumber}"}; };

    private:
        void DoGet() final;
        void DoPost() final;
        void DoPut() final;
        void DoDelete() final;
        InventoryDB     &DB_;
    };
}


#endif //OWPROV_RESTAPI_INVENTORY_HANDLER_H