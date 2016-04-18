/*
 * The MIT License (MIT)
 * 
 * All contributions by Krzysztof Narkiewicz (see https://github.com/ezaquarii/bison-flex-cpp-example for the original file):
 * Copyright (c) 2014 Krzysztof Narkiewicz <krzysztof.narkiewicz@ezaquarii.com>
 *
 * All other contributions:
 * Copyright (c) 2015,2016 Lukas Huwiler <lukas.huwiler@gmx.ch>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 */


#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <memory>
#include <regex>
#include <limits>
#include <unordered_set>
#include <osmium/osm/object.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/geom/haversine.hpp>
//#include <osmium/osm/tag.hpp>

namespace tagfilter {

class NumericCommand {
public:
  virtual std::shared_ptr<double> execute(const osmium::OSMObject& obj) = 0;
};

class NumericIdentity : public NumericCommand {
public:
 
  NumericIdentity(double val) {
    mValue = std::make_shared<double>(val);
  } 
  
  std::shared_ptr<double> execute(const osmium::OSMObject& obj) {
    return mValue;
  }
  
private:
  std::shared_ptr<double> mValue;
};

class HaversineDistance : public NumericCommand {
public:
  
  HaversineDistance(osmium::Location& loc) {
    mLocation = loc; 
  } 
  
  std::shared_ptr<double> execute(const osmium::OSMObject& obj) {
    if(obj.type() == osmium::item_type::node) {
      const osmium::Node& node = static_cast<const osmium::Node&>(obj);
      double res = osmium::geom::haversine::distance(node.location(), mLocation);
      return std::make_shared<double>(res);
    } 
    return nullptr;
  }
  
private:
  osmium::Location mLocation; 
};

class Command {
public:
  virtual bool execute(const osmium::OSMObject& obj) = 0;
  
  virtual void clear() {
  }
  
  virtual bool requiresAllEntities() {
    return false; 
  }
};

class CommandBoundingBox : public Command {
public:
  
  CommandBoundingBox(double min_lon, double min_lat, double max_lon, double max_lat) {
    mMinLon = min_lon;
    mMaxLon = max_lon;
    mMinLat = min_lat;
    mMaxLat = max_lat;
    mNodesWithinBox = std::make_shared<std::unordered_set<osmium::object_id_type>>();
    mWaysWithinBox = std::make_shared<std::unordered_set<osmium::object_id_type>>();
    mRelationsWithinBox = std::make_shared<std::unordered_set<osmium::object_id_type>>();
  } 
  
  bool execute(const osmium::OSMObject& obj) {
    bool ret = false;
    switch(obj.type()) {
    case osmium::item_type::node:
      {
        const osmium::Node& node = static_cast<const osmium::Node&>(obj);
        ret = isNodeWithinBox(node);
        break;
      }
    case osmium::item_type::way:
      {
        const osmium::Way& way = static_cast<const osmium::Way&>(obj);
        ret = isWayWithinBox(way);
        break;
      }
    case osmium::item_type::relation:
      {
        const osmium::Relation& rel = static_cast<const osmium::Relation&>(obj);
        ret = isRelationWithinBox(rel);
        break;
      }
    }
    return ret;
  } 
  
  void clear() {
    mNodesWithinBox->clear();
    mWaysWithinBox->clear();
    mRelationsWithinBox->clear();
  }
  
  bool requiresAllEntities() {
    return true;
  }
  
private:
  
  bool isNodeWithinBox(const osmium::Node& node) {
      bool within_lon = node.location().lon() <= mMaxLon && node.location().lon() >= mMinLon;
      bool within_lat = node.location().lat() <= mMaxLat && node.location().lat() >= mMinLat;
      bool within = within_lon && within_lat; 
      if(within) {
        mNodesWithinBox->insert(node.id());
      }   
      return within;
  } 
  
  bool isWayWithinBox(const osmium::Way& way) {
    bool ret = false;
    for(const osmium::NodeRef& nr : way.nodes()) {
      if(mNodesWithinBox->count(nr.ref()) > 0) {
        mWaysWithinBox->insert(way.id());
        ret = true;      
        break;
      }
    }   
    return ret;
  }
  
  bool isRelationWithinBox(const osmium::Relation& rel) {
    bool ret = false; 
    for(const osmium::RelationMember& rm : rel.members()) {
      switch(rm.type()) {
      case osmium::item_type::node:
        {
          if(mNodesWithinBox->count(rm.ref()) > 0) {
            ret = true;
          }
          break;
        }
      case osmium::item_type::way:
        {
          if(mWaysWithinBox->count(rm.ref()) > 0) {
            ret = true;
          }
          break;
        }
      case osmium::item_type::relation:
        {
          if(mRelationsWithinBox->count(rm.ref()) > 0) {
            ret = true;
          }
          break;
        }
      }
      if(ret) {
        mRelationsWithinBox->insert(rel.id());
        break;
      }
    }
    return ret;
  }
  
  double mMinLat;
  double mMaxLat;
  double mMinLon;
  double mMaxLon;
  std::shared_ptr<std::unordered_set<osmium::object_id_type>> mNodesWithinBox;
  std::shared_ptr<std::unordered_set<osmium::object_id_type>> mWaysWithinBox;
  std::shared_ptr<std::unordered_set<osmium::object_id_type>> mRelationsWithinBox; 
};

class CommandCompareId : public Command {
  
public:
  CommandCompareId(osmium::object_id_type id, osmium::item_type item_type) {
    mItemType = item_type;
    mId = id; 
  } 
  
  bool execute(const osmium::OSMObject& obj) {
    if(obj.id() == mId && obj.type() == mItemType) {
      return true; 
    }
    return false;
  } 
  
private:
  osmium::item_type mItemType;
  osmium::object_id_type mId; 
};

class CommandEqualValue : public Command {

public:
	CommandEqualValue(std::string val) {
		mComparisonValue = val;
	}
  
  bool execute(const osmium::OSMObject& obj) {
    return std::any_of(obj.tags().cbegin(), obj.tags().cend(), [this](const osmium::Tag& t) {
      return executeSingleTag(t);
    });
  }
  
  bool executeSingleTag(const osmium::Tag& tag) {
    return tag.value() == mComparisonValue; 
  }

private:
	std::string mComparisonValue;

};

class CommandEqualKey : public Command {

public:
	CommandEqualKey(std::string key) {
		mComparisonKey = key;
	}
  
  bool execute(const osmium::OSMObject& obj) {
    return std::any_of(obj.tags().cbegin(), obj.tags().cend(), [this](const osmium::Tag& t) {
      return executeSingleTag(t);
    });
  }
  
  bool executeSingleTag(const osmium::Tag& tag) {
    return tag.key() == mComparisonKey;
  }

private:
	std::string mComparisonKey;
};

class CommandMatchesValue : public Command {

public:
	CommandMatchesValue(std::string pattern) {
		mPattern = std::regex(pattern);
	}
  
  bool execute(const osmium::OSMObject& obj) {
    return std::any_of(obj.tags().cbegin(), obj.tags().cend(), [this](const osmium::Tag& t) {
      return std::regex_match(t.value(), mPattern);
    });
  }

private:
	std::regex mPattern;

};

class CommandMatchesKey : public Command {

public:
	CommandMatchesKey(std::string pattern) {
		mPattern = std::regex(pattern);
	}
  
  bool execute(const osmium::OSMObject& obj) {
    return std::any_of(obj.tags().cbegin(), obj.tags().cend(), [this](const osmium::Tag& t) {
      return std::regex_match(t.key(), mPattern);
    });
  }

private:
	std::regex mPattern;

};



class CommandIdenticalTag : public Command {

public:
	CommandIdenticalTag(std::string key, std::string val) : mKeyCompare(key), mValCompare(val) {}
  
  bool execute(const osmium::OSMObject& obj) {
    return std::any_of(obj.tags().cbegin(), obj.tags().cend(), [this](const osmium::Tag& t) {
      return mKeyCompare.executeSingleTag(t) && mValCompare.executeSingleTag(t);
    });
  }

private: 
	CommandEqualKey mKeyCompare;
	CommandEqualValue mValCompare;
};

class CommandNot : public Command {

public:
	CommandNot(std::shared_ptr<Command> cmd) {
		mCommand = cmd;
	}
  
  bool execute(const osmium::OSMObject& obj) {
    return !mCommand->execute(obj);
  }
  
  void clear() {
    mCommand->clear();
  }
  
  bool requiresAllEntities() {
    return mCommand->requiresAllEntities();
  }

private:
	std::shared_ptr<Command> mCommand;
};

class CommandAnd : public Command {

public:
	CommandAnd(std::shared_ptr<Command> first, std::shared_ptr<Command> second) {
		mFirst = first;
		mSecond = second;	
	}

 	bool execute(const osmium::OSMObject& obj) {
		return mFirst->execute(obj) && mSecond->execute(obj);
	}
  
  void clear() {
    mFirst->clear();
    mSecond->clear();
  }
  
  bool requiresAllEntities() {
    return mFirst->requiresAllEntities() || mSecond->requiresAllEntities();
  }

private:
	std::shared_ptr<Command> mFirst;
	std::shared_ptr<Command> mSecond;
};

class CommandOr : public Command {

public:
	CommandOr(std::shared_ptr<Command> first, std::shared_ptr<Command> second) {
		mFirst = first;
		mSecond = second;	
	}
  
	bool execute(const osmium::OSMObject& obj) {
		return mFirst->execute(obj) || mSecond->execute(obj);
	}
  
  void clear() {
    mFirst->clear();
    mSecond->clear();
  }
  
  bool requiresAllEntities() {
    return mFirst->requiresAllEntities() || mSecond->requiresAllEntities();
  }

private:
	std::shared_ptr<Command> mFirst;
	std::shared_ptr<Command> mSecond;
};

class CommandEqual : public Command {
  
public:
  CommandEqual(std::shared_ptr<NumericCommand> first, std::shared_ptr<NumericCommand> second) {
    mFirst = first;
    mSecond = second;
  } 
  
  bool execute(const osmium::OSMObject& obj) {
    std::shared_ptr<double> res1 = mFirst->execute(obj);
    std::shared_ptr<double> res2 = mSecond->execute(obj);
    if(res1 != nullptr && res2 != nullptr) {
      return *(res1) == *(res2);   
    }
    return true; 
  }
  
private:
  std::shared_ptr<NumericCommand> mFirst;
  std::shared_ptr<NumericCommand> mSecond; 
};

class CommandLess : public Command {
  
public:
  CommandLess(std::shared_ptr<NumericCommand> first, std::shared_ptr<NumericCommand> second) {
    mFirst = first;
    mSecond = second;
  } 
  
  bool execute(const osmium::OSMObject& obj) {
    std::shared_ptr<double> res1 = mFirst->execute(obj);
    std::shared_ptr<double> res2 = mSecond->execute(obj);
    if(res1 != nullptr && res2 != nullptr) {
      return *(res1) < *(res2);   
    }
    return true; 
  }
 
private:
  std::shared_ptr<NumericCommand> mFirst;
  std::shared_ptr<NumericCommand> mSecond; 
};

class CommandLEqual : public Command {
  
public:
  CommandLEqual(std::shared_ptr<NumericCommand> first, std::shared_ptr<NumericCommand> second) {
    mFirst = first;
    mSecond = second;
  } 
  
  bool execute(const osmium::OSMObject& obj) {
    std::shared_ptr<double> res1 = mFirst->execute(obj);
    std::shared_ptr<double> res2 = mSecond->execute(obj);
    if(res1 != nullptr && res2 != nullptr) {
      return *(res1) <= *(res2);   
    }
    return true; 
  }
  
private:
  std::shared_ptr<NumericCommand> mFirst;
  std::shared_ptr<NumericCommand> mSecond; 
};

class CommandGreater : public Command {
  
public:
  CommandGreater(std::shared_ptr<NumericCommand> first, std::shared_ptr<NumericCommand> second) {
    mFirst = first;
    mSecond = second;
  }
  
  bool execute(const osmium::OSMObject& obj) {
    std::shared_ptr<double> res1 = mFirst->execute(obj);
    std::shared_ptr<double> res2 = mSecond->execute(obj);
    if(res1 != nullptr && res2 != nullptr) {
      return *(res1) > *(res2);   
    }
    return true;
  }
  
private:
  std::shared_ptr<NumericCommand> mFirst;
  std::shared_ptr<NumericCommand> mSecond;
};

class CommandGEqual : public Command {
  
public:
  CommandGEqual(std::shared_ptr<NumericCommand> first, std::shared_ptr<NumericCommand> second) {
    mFirst = first;
    mSecond = second;
  } 
  
  bool execute(const osmium::OSMObject& obj) {
    std::shared_ptr<double> res1 = mFirst->execute(obj);
    std::shared_ptr<double> res2 = mSecond->execute(obj);
    if(res1 != nullptr && res2 != nullptr) {
      return *(res1) >= *(res2);   
    }
    return true; 
  }
 
private:
  std::shared_ptr<NumericCommand> mFirst;
  std::shared_ptr<NumericCommand> mSecond; 
};

}

#endif // COMMAND_H
