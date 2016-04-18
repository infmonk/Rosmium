\name{osm_apply}
\alias{osm_apply}

\title{
Applying Callback Functions to OSM Objects
}

\description{
This function allows to execute callback functions on OSM objects.
}

\usage{
osm_apply(reader, max_results = 1e+06, object_includes = "all", node_func = NULL, way_func = NULL, 
          rel_func = NULL, area_func = NULL, filter = NULL)
}

\arguments{
  \item{reader}{
    A reader object (see )
  }
  \item{max_results}{
    The maximum number of OSM objects passed to the callback function. This can be specified in order to prevent
    memory exhaustion. Default value is 1'000'000.
  }
  \item{object_includes}{
    Specifies the attributes of OSM objects passed to the \R side. Possible values are \kbd{"all"},\kbd{"id"},\kbd{"tags"},\kbd{"location"},\kbd{"geom"},\kbd{"node_refs"},\kbd{"members"}.
  }
  \item{node_func}{
    A callback function which is called on every node object (provided that the object satisfies the filter condition).
    The return value of the function is added to the result list. If no function is specified (default), all nodes
    are ignored.
  }
  \item{way_func}{
    A callback function which is called on every way object (provided that the object satisfies the filter condition).
    The return value of the function is added to the result list. If no function is specified (default), all ways
    are ignored.
  }
  \item{rel_func}{
    A callback function which is called on every relation object (provided that the object satisfies the filter condition).
    The return value of the function is added to the result list. If no function is specified (default), all relations
    are ignored.
  }
  \item{area_func}{
    A callback function which is called on every area object (provided that the object satisfies the filter condition).
    The return value of the function is added to the result list. If no function is specified (default), all areas
    are ignored. Areas are a helpful expansion of the osmium library and are not native OSM objects. 
  }
  \item{filter}{
    A filter object in order to filter out the relevant objects (see \code{\link[Rosmium]{object_filter}}).
  }
}
\details{

}

\value{
A list containing the results of all function calls.
}

\references{
}

\author{
Lukas Huwiler \email{lukas.huwiler@gmx.ch}
}

\seealso{

}

\examples{
\dontrun{
# Extract the city border of Bern and plot the city area
if(require(sp) && require(wkb)) {
  example_file <- system.file("osm_example/bern_switzerland.osm.pbf", package = "Rosmium")
  reader <- new(Reader, example_file, EntityBits.area)
  
  # Filter for border extraction
  border <- object_filter(t("admin_level","8") & t("name","Bern"))
  # Area callback: converts area to sp object
  getSpatialObject <- function(x) {
      wkb <- hex2raw(x$geom)
      readWKB(wkb)
  }
  
  # Get spatial object
  city_boundary <- osm_apply(reader, area_func = getSpatialObject, filter = border)
  
  # Plot the city area
  plot(city_boundary[[1]], density = -1, col = "goldenrod3", border = "black")
  
} else {
  stop("You need the packages wkb and sp in order to run this example") 
}
}
}
