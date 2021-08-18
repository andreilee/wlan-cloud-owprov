//
// Created by stephane bourque on 2021-08-16.
//

#include "RESTAPI_entity_handler.h"
#include "RESTAPI_ProvObjects.h"
#include "StorageService.h"

#include "Poco/JSON/Parser.h"
#include "Daemon.h"

namespace OpenWifi{

    void RESTAPI_entity_handler::handleRequest(Poco::Net::HTTPServerRequest &Request,
                                               Poco::Net::HTTPServerResponse &Response) {
        if (!ContinueProcessing(Request, Response))
            return;

        if (!IsAuthorized(Request, Response))
            return;

        ParseParameters(Request);
        if(Request.getMethod() == Poco::Net::HTTPRequest::HTTP_GET)
            DoGet(Request, Response);
        else if (Request.getMethod() == Poco::Net::HTTPRequest::HTTP_POST)
            DoPost(Request, Response);
        else if (Request.getMethod() == Poco::Net::HTTPRequest::HTTP_DELETE)
            DoDelete(Request, Response);
        else if (Request.getMethod() == Poco::Net::HTTPRequest::HTTP_PUT)
            DoPut(Request, Response);
        else
            BadRequest(Request, Response, "Unknown HTTP Method");
    }

    void RESTAPI_entity_handler::DoGet(Poco::Net::HTTPServerRequest &Request,
                                                 Poco::Net::HTTPServerResponse &Response) {
        try {
            std::string UUID = GetBinding("uuid", "");

            if(UUID.empty()) {
                BadRequest(Request, Response, "Missing UUID");
                return;
            }

            ProvObjects::Entity E;
            if(Storage()->EntityDB().GetRecord("id",UUID,E)) {
                Poco::JSON::Object Answer;
                E.to_json(Answer);
                ReturnObject(Request,Answer,Response);
                return;
            }
            NotFound(Request,Response);
            return;
        } catch (const Poco::Exception &E) {
            Logger_.log(E);
        }
        BadRequest(Request, Response);
    }

    void RESTAPI_entity_handler::DoDelete(Poco::Net::HTTPServerRequest &Request,
                                                    Poco::Net::HTTPServerResponse &Response) {
        try {
            std::string UUID = GetBinding("uuid", "");

            if(UUID.empty()) {
                BadRequest(Request, Response, "Missing UUID");
                return;
            }

            if(UUID == EntityDB::RootUUID()) {
                BadRequest(Request, Response, "Root Entity cannot be removed, only modified");
                return;
            }

            ProvObjects::Entity E;

            std::string f{"id"};
            if(!Storage()->EntityDB().GetRecord(f,UUID,E)) {
                NotFound(Request, Response);
                return;
            }

            if(!E.children.empty()) {
                BadRequest(Request, Response, "Entity still has children.");
                return;
            }

            if(Storage()->EntityDB().DeleteRecord(f,UUID)) {
                Storage()->EntityDB().DeleteChild(f,E.parent,UUID);
                OK(Request, Response);
                return;
            }
            NotFound(Request,Response);
            return;
        } catch (const Poco::Exception &E) {
            Logger_.log(E);
        }
        BadRequest(Request, Response);
    }

    void RESTAPI_entity_handler::DoPost(Poco::Net::HTTPServerRequest &Request,
                                                  Poco::Net::HTTPServerResponse &Response) {
        try {
            std::string UUID = GetBinding("uuid", "");

            if(UUID.empty()) {
                BadRequest(Request, Response, "Missing UUID");
                return;
            }

            if(!Storage()->EntityDB().RootExists() && UUID != EntityDB::RootUUID()) {
                BadRequest(Request, Response, "Root entity must be created first.");
                return;
            }

            Poco::JSON::Parser IncomingParser;
            Poco::JSON::Object::Ptr Obj = IncomingParser.parse(Request.stream()).extract<Poco::JSON::Object::Ptr>();
            ProvObjects::Entity E;
            if (!E.from_json(Obj)) {
                BadRequest(Request, Response);
                return;
            }

            //  When creating an entity, it cannot have any relations other that parent, notes, name, description. Everything else
            //  must be conveyed through PUT.
            E.info.id = (UUID==EntityDB::RootUUID()) ? UUID : uCentral::Daemon()->CreateUUID() ;
            if(UUID==EntityDB::RootUUID())
                E.parent="";
            else if(E.parent.empty()) {
                BadRequest(Request, Response, "Parent UUID must be specified");
                return;
            } else {
                std::string f{"id"};
                if(!Storage()->EntityDB().Exists(f,E.parent)) {
                    BadRequest(Request, Response, "Parent UUID must exist");
                    return;
                }
            }

            E.venues.clear();
            E.children.clear();
            E.managers.clear();
            E.contacts.clear();
            E.locations.clear();

            if(Storage()->EntityDB().CreateRecord(E)) {
                if(UUID==EntityDB::RootUUID())
                    Storage()->EntityDB().CheckForRoot();
                else {
                    std::string f{"id"};
                    Storage()->EntityDB().AddChild(f,E.parent,E.info.id);
                }

                OK(Request, Response);
                return;
            }
            NotFound(Request,Response);
            return;
        } catch (const Poco::Exception &E) {
            Logger_.log(E);
        }
        BadRequest(Request, Response);
    }

    void RESTAPI_entity_handler::DoPut(Poco::Net::HTTPServerRequest &Request,
                                                 Poco::Net::HTTPServerResponse &Response) {}
}