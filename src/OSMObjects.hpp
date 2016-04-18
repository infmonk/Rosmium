
// Rosmium: R bindings for the Osmium library
// Copyright (C) 2015 Lukas Huwiler
//
// This file is part of Rosmium.
//
// Rosmium is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Rosmium is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Rosmium.  If not, see <http://www.gnu.org/licenses/>.

#ifndef OSMOBJECTS_HPP
#define OSMOBJECTS_HPP

#include <Rcpp.h>
#include <osmium/osm/object.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/geom/factory.hpp>
#include <osmium/geom/wkb.hpp>

//Rcpp::NumericVector getLocation(const Rcpp::XPtr<const osmium::Node>& node) {
//  osmium::Location loc = node->location();
//  // SEXP ret = Rcpp::NumericVector::create(Rcpp::Named("lon") = loc.lon(), Rcpp::Named("lat") = loc.lat());
//  // return ret;
//  return Rcpp::NumericVector::create(Rcpp::Named("lon") = loc.lon(), Rcpp::Named("lat") = loc.lat());
//}

class RosmiumWrapper {
  
public:

  RosmiumWrapper(Rcpp::CharacterVector& obj_includes) {
    for(auto object_include : obj_includes) {
      if(object_include == "id" || object_include == "all") {
        mIncludeId = true;
      }
      if(object_include == "tags" || object_include == "all") {
        mIncludeTags = true;
      }  
      if(object_include == "location" || object_include == "all") {
        mIncludeLocation = true;
      } 
      if(object_include == "geom" || object_include == "all") {
        mGeomFactory = std::make_shared<osmium::geom::WKBFactory<>>(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex); 
      }  
      if(object_include == "node_refs" || object_include == "all") {
        mIncludeNodeRefs = true;
      }  
      if(object_include == "members" || object_include == "all") {
        mIncludeMembers = true;
      }
    }
  } 
  
  Rcpp::List createRNode(const osmium::Node& node) {
    Rcpp::List ret(4);
    
    if(mIncludeId) {
      ret[0] = getId(node);
    }
    if(mIncludeTags) {
      ret[1] = getTags(node);
    }
    if(mIncludeLocation) {
      ret[2] = getLocation(node); 
    }
    if(mGeomFactory != nullptr) {
      ret[3] = createWKB(node);
    }
    ret.attr("names") = Rcpp::CharacterVector::create("id","tags","location","geom");
    ret.attr("class") = "node";
    return ret;
  }
  
  Rcpp::List createRWay(const osmium::Way& way) {
    Rcpp::List ret(4);
    if(mIncludeId) {
      ret[0] = getId(way);
    }
    if(mIncludeTags) {
      ret[1] = getTags(way);
    }
    if(mIncludeNodeRefs) {
      ret[2] = getNodeRefs(way);
    }
    if(mGeomFactory != nullptr) {
      ret[3] = createWKB(way);
    }
    ret.attr("names") = Rcpp::CharacterVector::create("id","tags","node_refs","geom");
    ret.attr("class") = "way";
    return ret;
  } 
 
  Rcpp::List createRRelation(const osmium::Relation& rel) {
    Rcpp::List ret(3);
    
    if(mIncludeId) {
      ret[0] = getId(rel); 
    }
    if(mIncludeTags) {
      ret[1] = getTags(rel); 
    }
    if(mIncludeMembers) {
      ret[2] = getRelMembers(rel); 
    }
    ret.attr("names") = Rcpp::CharacterVector::create("id","tags","members"); 
    ret.attr("class") = "relation";
    return ret;
  } 
  
  Rcpp::List createRArea(const osmium::Area& area) {
    Rcpp::List ret = Rcpp::List::create(Rcpp::Named("id") = getId(area), Rcpp::Named("tags") = getTags(area),
                                        Rcpp::Named("geom") = createWKB(area));
    ret.attr("class") = "area";
    return ret;   
  }
  
private:
  
  std::shared_ptr<osmium::geom::WKBFactory<>> mGeomFactory = nullptr;
  bool mIncludeId = false;
  bool mIncludeTags = false;
  bool mIncludeLocation = false;
  bool mIncludeNodeRefs = false;
  bool mIncludeMembers = false;

  Rcpp::CharacterVector getId(const osmium::OSMObject& obj) {
    return Rcpp::CharacterVector::create(std::to_string(obj.id()));
  }
  
  Rcpp::CharacterMatrix getTags(const osmium::OSMObject& obj) {
    const osmium::TagList& tags = obj.tags();
    Rcpp::CharacterMatrix ret(tags.size(), 2);
    colnames(ret) = Rcpp::CharacterVector::create("key","value");
    int row = 0;
    for(auto it = tags.cbegin(); it != tags.cend(); ++it) {
      ret(row, 0) = Rcpp::String(it->key());
      ret(row, 1) = Rcpp::String(it->value());
      row++;
    } 
    return ret;
  } 
  
  Rcpp::NumericMatrix getNodeRefs(const osmium::Way& way) {
    Rcpp::NumericMatrix ret(way.nodes().size(), 2);
    std::fill(ret.begin(), ret.end(), Rcpp::NumericVector::get_na());
    int row = 0;
    colnames(ret) = Rcpp::CharacterVector::create("lon","lat");
    Rcpp::CharacterVector row_names = Rcpp::CharacterVector(ret.nrow());
    for(const osmium::NodeRef& nr : way.nodes()) {
      if(nr.location().valid()) {
        ret(row, 0) = nr.lon(); 
        ret(row, 1) = nr.lat();
      }
      row_names[row] = std::to_string(nr.ref());
      row++;
    }  
    rownames(ret) = row_names;
    return ret;
  }
  
  Rcpp::NumericVector getLocation(const osmium::Node& node) {
    const osmium::Location& loc = node.location();
    if(loc.valid()) {
      return Rcpp::NumericVector::create(Rcpp::Named("lon") = loc.lon_without_check(), Rcpp::Named("lat") = loc.lat_without_check());
    } else {
      return Rcpp::NumericVector::create(Rcpp::Named("lon") = NA_REAL, Rcpp::Named("lat") = NA_REAL);
    }
  } 
  
  Rcpp::CharacterMatrix getRelMembers(const osmium::Relation& rel) {
    const osmium::RelationMemberList& members = rel.members(); 
    Rcpp::CharacterMatrix ret(members.size(), 2);
    std::fill(ret.begin(), ret.end(), Rcpp::CharacterVector::get_na());
    colnames(ret) = Rcpp::CharacterVector::create("entity_type","role");
    Rcpp::CharacterVector row_names = Rcpp::CharacterVector(members.size());
    int row = 0;
    for(const osmium::RelationMember& rm : members) {
      if(rm.type() == osmium::item_type::way) {
        ret(row, 0) = "way";
      } else if (rm.type() == osmium::item_type::node) {
        ret(row, 0) = "node";
      } else if (rm.type() == osmium::item_type::relation) {
        ret(row, 0) = "relation";
      }
      ret(row, 1) = rm.role();
      row_names[row] = std::to_string(rm.ref());
      row++; 
    }
    rownames(ret) = row_names;
    return ret;
  }
  
  Rcpp::CharacterVector createWKB(const osmium::Node& node) {
    Rcpp::CharacterVector ret(1);
    try {
      ret[0] = mGeomFactory->create_point(node);
      ret.attr("class") = "wkb";
      return ret; 
    } catch(std::exception& e) {
      ret[0] = e.what();
      ret.attr("class") = "invalid_geometry";
      return ret;
    }
  }
  
  Rcpp::CharacterVector createWKB(const osmium::Way& way) {
//    if(way.is_closed()) {
//      // return mGeomFactory->create_polygon(way);
//    } else {
    Rcpp::CharacterVector ret(1);
    try {
      ret[0] = mGeomFactory->create_linestring(way); 
      ret.attr("class") = "wkb";
      return ret; 
    } catch(std::exception& e) {
      ret[0] = e.what();
      ret.attr("class") = "invalid_geometry";
      return ret;
    }
//    }
  }
  
  Rcpp::CharacterVector createWKB(const osmium::Area& area) {
    Rcpp::CharacterVector ret(1);
    try {
      ret[0] = mGeomFactory->create_multipolygon(area);
      ret.attr("class") = "wkb"; 
      return ret; 
    } catch(std::exception& e) {
      ret[0] = e.what();
      ret.attr("class") = "invalid_geometry";
      return ret;
    }
  } 
  
};

#endif // OSMOBJECTS_HPP