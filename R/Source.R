
## Rosmium: R bindings for the Osmium library
## Copyright (C) 2015,2016 Lukas Huwiler
## 
## This file is part of Rosmium.
## 
## Rosmium is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 2 of the License, or
## (at your option) any later version.
## 
## Rosmium is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with Rosmium.  If not, see <http://www.gnu.org/licenses/>.

object_filter <- function(expr, is_char = FALSE) {
  if(!is_char) {
    expr <- deparse(substitute(expr))
    expr <- paste(expr, collapse = "")
  }
  new(ObjectFilter, expr)
}

osm_apply <- function(reader, max_results = 1000000, object_includes = "all", node_func = NULL, way_func = NULL, rel_func = NULL, area_func = NULL, filter = NULL) {
  object_includes <- match.arg(object_includes, choices = c("all","id","tags","location","geom","node_refs","members"), TRUE)
  handler <- new(InternalRHandler, object_includes, result_size = max_results)
  result <- vector(mode = "list", length = max_results)
  last_res <- 0
  if(!is.null(node_func)) {
    handler$registerFunction(function(x,i) {
      last_res <<- i
      result[[i]] <<- node_func(x)
    }, EntityBits.node)
  }
  if(!is.null(way_func)) {
    handler$registerFunction(function(x,i) {
      last_res <<- i
      result[[i]] <<- way_func(x)
    }, EntityBits.way)
    
  }
  if(!is.null(rel_func)) {
    handler$registerFunction(function(x,i) {
      last_res <<- i
      result[[i]] <<- rel_func(x)
    }, EntityBits.relation) 
  }
  if(!is.null(area_func)) {
    handler$registerFunction(function(x,i) {
      last_res <<- i
      result[[i]] <<- area_func(x)
    }, EntityBits.area)
  }
  if(!is.null(filter)) {
    handler$registerObjectFilter(filter)
  }
  reader$applyR(handler, TRUE, "blah")
  if(last_res > 0) {
    return(result[1:last_res])
  }
}

#.registerFunction <- function(handler, entity, func = NULL) {
#  if(!is.null(func)) {
#    wrap_func <- function(x, i) {
#      last_res <<- i
#      result[[i]] <<- func(x)
#    }
#    # environment(wrap_func) <- parent.frame()
#    handler$registerFunction(wrap_func, entity)
#  }
#}

# dui <- createRHandler()
# 
# 
# setRefClass("Rcpp_InternalRHandler")
# 

#setOldClass(c("Rcpp_InternalRHandler"))
#
## setOldClass("Rcpp_InternalRHandler", where = "package:Rosmium")
#RHandler <- setRefClass("RHandler",
#                              fields = list(result = "list",
#                                            max_results = "numeric",
#                                            handler = "envRefClass"),
#                              methods = list(
#                                initialize = function(max_results) {
#                                  .self$max_results <- max_results
#                                  if(!is.na(max_results)) {
#                                    print("blah")
#                                    .self$result <- vector(mode = "list", length = max_results) 
#                                  } else {
#                                    .self$result <- list()
#                                  }
#                                  .self$handler <- new(InternalRHandler, max_results = max_results)
#                                },
#                                registerFunction = function(func, entity_types) {
##                                   wrap_fun <- function(x, i) {
##                                     print(i)
##                                     .self$result[[i]] <- func(x)
##                                     # names(result)[i] <<- paste0(names(entity_types), id(x))
##                                   }
#                                  environment(func) <- environment()
#                                  # callSuper(wrap_fun, entity_types)
#                                  .self$handler$registerFunction(func, entity_types)
#                                }
#                              )
#)

#setMethod("apply",c(X = "RHandler", MARGIN = "missing", FUN = "Reader"),
#          function(X, MARGIN, FUN, reader, ...) {
#            reader$applyR(handler$handler)
#          }
#)
# setMethod("registerFunction", function(func, entity_types) {
#   environment(func) 
#   wrap_func <- function(x, i) {
#     
#   } 
# })
#
# RHandler$methods(
#   registerFunction = function(func, entity_types) {
#     wrap_fun <- function(x, i) {
#       # .seld$result[[i]] <<- func(x)
#       # names(result)[i] <<- paste0(names(entity_types), id(x))
#     }
#     # callSuper(wrap_fun, entity_types)
#   }
# )
#

#setMethod("initialize", "Rcpp_Tag", function(.Object, ...) stop("No public constructor for class Tag"))
#setMethod("initialize", "Rcpp_OSMObject", function(.Object, ...) stop("No public constructor for class OSMObject"))
#setMethod("initialize", "Rcpp_Node", function(.Object, ...) stop("No public constructor for class Node"))

# setMethod("[", c(x = "Rcpp_HandlerResult", i = "numeric", j = "missing"),
#           function(x, i, j, ..., drop = TRUE) {
#             x$get(i)
#           })

# setMethod("initialize", "Rcpp_InternalRHandler", function(.Object, ...) stop("Blah"))

#createRHandler <- function(max_size = 1000000) {
#  new(RHandler, result_size = max_size) 
#}


