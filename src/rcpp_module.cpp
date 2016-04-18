
// Rosmium: R bindings for the Osmium library
// Copyright (C) 2015,2016 Lukas Huwiler
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

#include <Rcpp.h>
#include <memory>
#include <unordered_set>
#include <math.h>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/object.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/io/pbf_input.hpp>
#include <osmium/io/xml_output.hpp>
#include <osmium/io/pbf_output.hpp>
#include <osmium/io/writer.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/index/map/all.hpp>
#include <osmium/index/node_locations_map.hpp>
#include <map>


#include "object_filter/interpreter.h"
#include "OSMObjects.hpp"

RCPP_EXPOSED_CLASS(OSMReader)
RCPP_EXPOSED_CLASS(CountHandler)
RCPP_EXPOSED_CLASS(RHandler)
RCPP_EXPOSED_CLASS(WriteHandler)
RCPP_EXPOSED_CLASS(Dummy)
RCPP_EXPOSED_CLASS(ObjectFilter)
  
typedef std::map<osmium::osm_entity_bits::type, Rcpp::Function> EntityFunctionMap;
typedef std::pair<osmium::osm_entity_bits::type, Rcpp::Function> EntityFunctionPair;
typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location> sparse_mem_array;
typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> index_type;

class ParseException : public std::exception
{
  virtual const char* what() const throw()
  {
    return "Parse error, the provided filter expression is not syntactically valid (see ?tagfilter)";
  }
} ParseEx;

class ObjectFilter {
public:
  ObjectFilter(Rcpp::CharacterVector filter_expr) {
    tagfilter::Interpreter i;
    std::string expr = Rcpp::as<std::string>(filter_expr);
    try {
      if(!i.parse(expr)) {
        mCommand = i.returnAST();
      } else {
        throw ParseEx; 
      }
    } catch(ParseException &ex) {
      Rcpp::stop(i.getError());
    }
  } 
  
  std::shared_ptr<tagfilter::Command> getCommand() {
    return mCommand;
  }
private:
  std::shared_ptr<tagfilter::Command> mCommand = nullptr;
};  
  
struct CountHandler : public osmium::handler::Handler {
  uint64_t nodes = 0;
  uint64_t ways = 0;
  uint64_t relations = 0;
  
  void node(osmium::Node&) {
    ++nodes;
  }
  
  void way(osmium::Way&) {
    ++ways;
  }
  
  void relation(osmium::Relation&) {
    ++relations;
  }
  
};

class HandlerWithFilter : public osmium::handler::Handler {
public:
  
  inline void registerObjectFilter(ObjectFilter& filter) {
    mObjectFilter = filter.getCommand();
  } 
  
  inline void setObjectFilter(std::shared_ptr<tagfilter::Command> filter) {
    mObjectFilter = filter;
  }
  
  inline std::shared_ptr<tagfilter::Command> getFilter() {
    return mObjectFilter;
  }
  
  inline bool meetsFilterCondition(const osmium::OSMObject& obj) {
    return mObjectFilter == nullptr || mObjectFilter->execute(obj);
  }  
  
  void clearFilter() {
    if(mObjectFilter != nullptr) {
      mObjectFilter->clear();
    }
  } 
  
  bool requiresAllEntities() {
    bool ret = false;
    if(mObjectFilter != nullptr) {
      ret = mObjectFilter->requiresAllEntities();
    }
    return ret;
  }
  
private:
   std::shared_ptr<tagfilter::Command> mObjectFilter = nullptr; 
};

class WriteHandler : public HandlerWithFilter {
  
public:
  
  WriteHandler(std::string filename) {
    mFilename = filename;
    // mWriter = std::make_shared<osmium::io::Writer>(filename); 
  } 
  
  void init() {
    mWriter = std::make_shared<osmium::io::Writer>(mFilename); 
    mNodeRefs = std::make_shared<std::unordered_set<osmium::object_id_type>>();
    mWayRefs = std::make_shared<std::unordered_set<osmium::object_id_type>>();
    mRelRefs = std::make_shared<std::unordered_set<osmium::object_id_type>>();
  }
  
  void close() {
    clearFilter();
    mWriter->close();
    mWriter = nullptr;
    mNodeRefs = nullptr;
    mWayRefs = nullptr;
    mRelRefs = nullptr; 
  }
  
  void node(const osmium::Node& node) {
    if(containsID(node.id(), mNodeRefs) || meetsFilterCondition(node)) {     
      (*mWriter)(node);     
    }
  }
  
  void way(const osmium::Way& way) {
    if(containsID(way.id(), mWayRefs) || meetsFilterCondition(way)) {
      (*mWriter)(way);
    }
  }

  void relation(const osmium::Relation& rel) {
    if(containsID(rel.id(), mRelRefs) || meetsFilterCondition(rel)) {
      (*mWriter)(rel); 
    }
  } 
  
  void addID(osmium::object_id_type id, osmium::osm_entity_bits::type object_type) {
    switch(object_type) {
    case osmium::osm_entity_bits::node:
      if(!containsID(id, mNodeRefs)) {
        mNodeRefs->insert(id);  
      }
      break;
    case osmium::osm_entity_bits::way:
      if(!containsID(id, mWayRefs)) {
        mWayRefs->insert(id);  
      }
      break;     
    case osmium::osm_entity_bits::relation:
      if(!containsID(id, mRelRefs)) {
        mRelRefs->insert(id);  
      }
      break;
    } 
  }
  
private:
  std::string mFilename;
  std::shared_ptr<osmium::io::Writer> mWriter; 
  std::shared_ptr<std::unordered_set<osmium::object_id_type>> mNodeRefs; 
  std::shared_ptr<std::unordered_set<osmium::object_id_type>> mWayRefs;
  std::shared_ptr<std::unordered_set<osmium::object_id_type>> mRelRefs;
  
  bool containsID(osmium::object_id_type id, std::shared_ptr<std::unordered_set<osmium::object_id_type>> ids) {
    return ids->count(id) > 0;
  }
};

class WriteHelper : public HandlerWithFilter {
public:
  
  WriteHelper(WriteHandler& wh) : mWriter(wh) {
    mWaysToDo = std::make_shared<std::unordered_set<osmium::object_id_type>>();
    mRelToDo = std::make_shared<std::unordered_set<osmium::object_id_type>>();   
    setObjectFilter(wh.getFilter());
  }
  
  void way(const osmium::Way& way) {
    bool in_todo = mWaysToDo->count(way.id()) > 0;
    if(in_todo || meetsFilterCondition(way)) {
      mWriter.addID(way.id(), osmium::osm_entity_bits::way); 
      if(in_todo) {
        mWaysToDo->erase(way.id());
      }
      for(const osmium::NodeRef& nr : way.nodes()) {
        mWriter.addID(nr.ref(), osmium::osm_entity_bits::node); 
      }
    } 
  }
  
  void relation(const osmium::Relation& rel) {
    bool in_todo = mRelToDo->count(rel.id()) > 0;
    if(in_todo || meetsFilterCondition(rel)) {
      mWriter.addID(rel.id(), osmium::osm_entity_bits::relation);
      if(in_todo) {
        mRelToDo->erase(rel.id());
      } 
      for(const osmium::RelationMember& rm : rel.members()) {
        if(rm.type() == osmium::item_type::way) {
          mWaysToDo->insert(rm.ref()); 
        } else if (rm.type() == osmium::item_type::node) {
          mWriter.addID(rm.ref(), osmium::osm_entity_bits::node);
        } else if (rm.type() == osmium::item_type::relation) {
          mRelToDo->insert(rm.ref());
        }
      }
    }
  }
  
  bool anyRelationsToDo() {
    return !mRelToDo->empty();
  }
  
  bool anyWaysToDo() {
    return !mWaysToDo->empty();
  }
  
private:
  std::shared_ptr<std::unordered_set<osmium::object_id_type>> mWaysToDo;
  std::shared_ptr<std::unordered_set<osmium::object_id_type>> mRelToDo;
  WriteHandler& mWriter; 
}; 

class RHandler : public osmium::handler::Handler {
public: 
  
  int mResultSize = 0;
  
  RHandler(Rcpp::CharacterVector object_includes, Rcpp::IntegerVector max_results) : mRWrapper(object_includes){
    mResultSize = Rcpp::as<int>(max_results);
  }
  
  void registerFunction(Rcpp::Function func, unsigned char object_types) {
    if(object_types & osmium::osm_entity_bits::node) { 
      setFunction(func, osmium::osm_entity_bits::node);
    }
    if(object_types & osmium::osm_entity_bits::way) { 
      setFunction(func, osmium::osm_entity_bits::way);
    }
    if(object_types & osmium::osm_entity_bits::relation) {
      setFunction(func, osmium::osm_entity_bits::relation); 
    }
    if(object_types & osmium::osm_entity_bits::area) {
      setFunction(func, osmium::osm_entity_bits::area); 
    }
  }
  
  void registerObjectFilter(ObjectFilter& filter) {
    mObjectFilter = filter.getCommand();
  }
  
  void node(const osmium::Node& node) {
    if(mFunctions.count(osmium::osm_entity_bits::node) && meetsFilterCondition(node) && mCurrentCount++ < mResultSize) {     
      (mFunctions.at(osmium::osm_entity_bits::node))(mRWrapper.createRNode(node), mCurrentCount);
    }
  }
  
  void way(const osmium::Way& way) {
    if(mFunctions.count(osmium::osm_entity_bits::way) && meetsFilterCondition(way) && mCurrentCount++ < mResultSize) {
        (mFunctions.at(osmium::osm_entity_bits::way))(mRWrapper.createRWay(way), mCurrentCount);
    }
  }

  void relation(const osmium::Relation& rel) {
    if(mFunctions.count(osmium::osm_entity_bits::relation) && meetsFilterCondition(rel) && mCurrentCount++ < mResultSize) {
      (mFunctions.at(osmium::osm_entity_bits::relation))(mRWrapper.createRRelation(rel), mCurrentCount);
    }
  }
  
  void area(const osmium::Area& area) {
    if(mFunctions.count(osmium::osm_entity_bits::area) && meetsFilterCondition(area) && mCurrentCount++ < mResultSize) {
      (mFunctions.at(osmium::osm_entity_bits::area))(mRWrapper.createRArea(area), mCurrentCount);
    }   
  }
  
  bool hasAreaCallback() {
    return mFunctions.count(osmium::osm_entity_bits::area) > 0;
  }
  
private:
  
  void setFunction(Rcpp::Function& func, osmium::osm_entity_bits::type object_type) {
      if(!mFunctions.count(object_type)) {
        mFunctions.insert(EntityFunctionPair(object_type, func));
      } else {
        mFunctions.at(object_type) = func;
      } 
  }
  
  inline bool meetsFilterCondition(const osmium::OSMObject& obj) {
    return mObjectFilter == nullptr || mObjectFilter->execute(obj);
  } 
  
  int mCurrentCount = 0;
  RosmiumWrapper mRWrapper;
  EntityFunctionMap mFunctions;
  std::shared_ptr<tagfilter::Command> mObjectFilter = nullptr;
};




void set_lon(osmium::Location* loc, double lon) {
 loc->set_lon(lon);
}

void set_lat(osmium::Location* loc, double lat) {
 loc->set_lat(lat);
}

class OSMReader {
  
private:
  std::string mFilename;
  osmium::osm_entity_bits::type mEntities;
 
// TODO: create index from string 
  std::unique_ptr<index_type> createIndex(const std::string& idx) {
    if(idx == "sparse_mem_array") return std::unique_ptr<index_type>(new sparse_mem_array());
  }
 
  void apply_with_location(RHandler& handler, osmium::io::Reader &r, const std::string &idx) {
    std::unique_ptr<index_type> index = std::unique_ptr<index_type>(new sparse_mem_array());
    osmium::handler::NodeLocationsForWays<index_type> location_handler(*index);
    location_handler.ignore_errors();
    osmium::apply(r, location_handler, handler);
  }
   
  void apply_with_area(RHandler& handler, osmium::io::Reader &r,
                       osmium::area::MultipolygonCollector<osmium::area::Assembler> &collector,
                       const std::string &idx) {
    std::unique_ptr<index_type> index = std::unique_ptr<index_type>(new sparse_mem_array());
    osmium::handler::NodeLocationsForWays<index_type> location_handler(*index);
    location_handler.ignore_errors();
    osmium::apply(r, location_handler, handler,
                  collector.handler([&handler](const osmium::memory::Buffer& area_buffer) {
                    osmium::apply(area_buffer, handler);
                  }));
  } 
  
public:
  OSMReader(const std::string filename, unsigned char read_which_entities) {
    mFilename = filename;
    mEntities = (osmium::osm_entity_bits::type) read_which_entities;
  }
  
  std::string getFilename() {
    return mFilename;
  }
  
  void apply(CountHandler& handler) {
    osmium::io::Reader reader(mFilename, mEntities);
    osmium::apply(reader, handler);
    reader.close();
  }
  
  void apply_r(RHandler& handler, bool with_locations = false, std::string idx = "sparse_mem_array") {
    if(handler.hasAreaCallback()) {
      osmium::area::Assembler::config_type assembler_config;
      osmium::area::MultipolygonCollector<osmium::area::Assembler> collector(assembler_config);
      osmium::io::Reader reader1(mFilename);
      collector.read_relations(reader1);
      reader1.close();
      osmium::io::Reader reader2(mFilename);
      apply_with_area(handler, reader2, collector, idx);
      reader2.close();
    } else if(with_locations) {
      osmium::io::Reader reader(mFilename, mEntities);
      apply_with_location(handler, reader, idx);
      reader.close();
    } else {
      osmium::io::Reader reader(mFilename, mEntities);
      osmium::apply(reader, handler);
      reader.close();
    }
  }
  
  void apply_writer(WriteHandler& handler, bool include_refs) {
    osmium::io::Reader reader(mFilename, mEntities);
    handler.init();
    if(include_refs) {
      WriteHelper wh(handler); 
      if(mEntities & osmium::osm_entity_bits::relation) {
        osmium::osm_entity_bits::type pre_pass = osmium::osm_entity_bits::nwr;
        if(!wh.requiresAllEntities()) {
          pre_pass = osmium::osm_entity_bits::relation;
        } 
        do {
          osmium::io::Reader relReader(mFilename, pre_pass); 
          osmium::apply(relReader, wh);
          relReader.close(); 
        } while(wh.anyRelationsToDo());
      }
      if(mEntities & osmium::osm_entity_bits::way) {
        osmium::osm_entity_bits::type pre_pass = osmium::osm_entity_bits::nwr;
        if(!wh.requiresAllEntities()) {
          pre_pass = osmium::osm_entity_bits::way;
        } 
        do {
          osmium::io::Reader wayReader(mFilename, pre_pass); 
          osmium::apply(wayReader, wh);
          wayReader.close(); 
        } while(wh.anyWaysToDo());
      }   
      wh.clearFilter();
    }
    osmium::apply(reader, handler);
    try {
      handler.close();
    } catch(std::exception e) {
      Rcpp::stop(e.what()); 
    }
    reader.close();
  }
  
};

class Dummy {
   int x;
   int get_x() {return x;}
};

RCPP_MODULE(Rosmium){
  using namespace Rcpp ;
  using namespace osmium;
  
  class_<Dummy>("EntityBits")
    .default_constructor()
  ;
  
  enum_<osm_entity_bits::type, Dummy>("EnumType")
    .value("nothing", osm_entity_bits::nothing)
    .value("node", osm_entity_bits::node)
    .value("way", osm_entity_bits::way)
    .value("relation", osm_entity_bits::relation)
    .value("nwr", osm_entity_bits::nwr)
    .value("area", osm_entity_bits::area)
    .value("nwra", osm_entity_bits::nwra)
    .value("object", osm_entity_bits::object)
    .value("changeset", osm_entity_bits::changeset)
    .value("all", osm_entity_bits::all)
  ;
  
  class_<OSMReader>("Reader")
    .constructor<std::string, unsigned char>()
    .property("file", &OSMReader::getFilename)
    .method("apply", &OSMReader::apply)
    .method("applyR", &OSMReader::apply_r)
    .method("apply_writer", &OSMReader::apply_writer)
  ;
  
  class_<osmium::handler::Handler>("Handler")
    ;
  
  class_<HandlerWithFilter>("FilterHandler")
    .derives<osmium::handler::Handler>("Handler")
    .method("registerObjectFilter", &HandlerWithFilter::registerObjectFilter)
  ;
  
  class_<RHandler>("InternalRHandler")
    .derives<osmium::handler::Handler>("Handler")
    .constructor<Rcpp::CharacterVector, Rcpp::IntegerVector>()
    .method("registerFunction", &RHandler::registerFunction)
    .method("registerObjectFilter", &RHandler::registerObjectFilter)
    .field("max_results", &RHandler::mResultSize)
  ;
  
  class_<WriteHandler>("WriteHandler")
    .derives<HandlerWithFilter>("FilterHandler")
    .constructor<std::string>()
  ;
  
  class_<CountHandler>("CountHandler")
    .derives<osmium::handler::Handler>("Handler")
    .default_constructor()
    .field("nodes",&CountHandler::nodes)
    .field("ways", &CountHandler::ways)
  ;
  
  class_<ObjectFilter>("ObjectFilter")
    .constructor<Rcpp::CharacterVector>()  
  ;
}


