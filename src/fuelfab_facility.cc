#include "fuelfab_facility.h"

namespace fuelfab {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    FuelfabFacility::FuelfabFacility(cyclus::Context* ctx)
        : cyclus::Facility(ctx) {};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    std::string FuelfabFacility::str() {
      return Facility::str();
    }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void FuelfabFacility::Tick() {
        if(inventory.count() == 0){
            inventory.set_capacity(maximum_storage);
        }
    }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void FuelfabFacility::Tock() {

    }

    std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr> FuelfabFacility::GetMatlRequests() {
        using cyclus::RequestPortfolio;
        using cyclus::Material;
        using cyclus::Composition;
        using cyclus::CompMap;
        using cyclus::CapacityConstraint;
        std::set<RequestPortfolio<Material>::Ptr> ports;
        cyclus::Context* ctx = context();
        CompMap cm;
        Material::Ptr target = Material::CreateUntracked(1,
                              Composition::CreateFromAtom(cm));
        RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
        double qty = inventory.space();
        for(int i = 0; i < in_commods.size(); i++){
            port->AddRequest(target, this, in_commods[i]);
        }
        CapacityConstraint<Material> cc(qty);

        port->AddConstraint(cc);
        ports.insert(port);
        return ports;
    }

    // MatlBids //
    std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>FuelfabFacility::GetMatlBids(
            cyclus::CommodMap<cyclus::Material>::type& commod_requests) {

        using cyclus::BidPortfolio;
        using cyclus::CapacityConstraint;
        using cyclus::Converter;
        using cyclus::Material;
        using cyclus::Request;
        using reactor::ReactorFacility;

        cyclus::Context* ctx = context();
        std::set<BidPortfolio<Material>::Ptr> ports;

        // respond to all requests of my commodity
        if (inventory.count() == 0){return ports;}

        std::vector<cyclus::Material::Ptr> manifest;
        manifest = cyclus::ResCast<Material>(inventory.PopN(inventory.count()));

        BidPortfolio<Material>::Ptr port(new BidPortfolio<Material>());
        std::vector<Request<Material>*>& requests = commod_requests[out_commod];
        std::vector<Request<Material>*>::iterator it;
        for (it = requests.begin(); it != requests.end(); ++it) {
            Request<Material>* req = *it;
            ReactorFacility* reactor = dynamic_cast<ReactorFacility*>(req->requester());
            if (reactor == NULL){
               throw cyclus::CastError("Nope!");
            } else {

                reactor.burnup_test()
            }
            if (req->commodity() == out_commod) {
                Material::Ptr offer = Material::CreateUntracked(1, manifest[0]->comp());
                port->AddBid(req, offer, this);
            }
        }
        inventory.PushAll(manifest);
        CapacityConstraint<Material> cc(1);

        port->AddConstraint(cc);
        ports.insert(port);
        return ports;
    }

    void FuelfabFacility::AcceptMatlTrades(const std::vector< std::pair<cyclus::Trade<cyclus::Material>,
                                            cyclus::Material::Ptr> >& responses) {

        std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                            cyclus::Material::Ptr> >::const_iterator it;
      for (it = responses.begin(); it != responses.end(); ++it) {
        inventory.Push(it->second);
      }
    }

    void FuelfabFacility::GetMatlTrades(const std::vector< cyclus::Trade<cyclus::Material> >& trades,
                                        std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                                        cyclus::Material::Ptr> >& responses) {
        using cyclus::Material;
        using cyclus::Trade;

        std::vector< cyclus::Trade<cyclus::Material> >::const_iterator it;
        cyclus::Material::Ptr discharge = cyclus::ResCast<Material>(inventory.Pop());
        for (it = trades.begin(); it != trades.end(); ++it) {
            responses.push_back(std::make_pair(*it, discharge));
        }
    }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    extern "C" cyclus::Agent* ConstructFuelfabFacility(cyclus::Context* ctx) {
      return new FuelfabFacility(ctx);
    }

}