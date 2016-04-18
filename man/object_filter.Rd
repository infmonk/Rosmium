\name{object_filter}
\alias{object_filter}

\title{
Filtering OSM Objects
}

\description{
This function generates a filter for fast filtering of OSM objects on the C++ side.
Most of the time, a user is only interested in a subset of OSM objects (e.g. all objects with a highway tag).
You could filter such objects within the callback function of a handler.
However, this is costly, since every OSM object has to be passed to the \R side.
Using an object filter allows you to filter the OSM objects on the C++ side.

In order to translate a filter to C++, a small language has been written.
Details on the syntax is provided in the section Details.
}

\usage{
object_filter(expr, is_char = FALSE)
}

\arguments{
  \item{expr}{
    A filter expression (see section Details for syntax specification). If \code{is_char = TRUE} the expression
    has to be an object of class character. 
  }
  \item{is_char}{
    Whether argument \code{expr} is a character object. 
  }
}

\details{
\subsection{numeric_expression}{
A \dfn{numeric expression} consists of:
\itemize{
  \item \bold{numeric}: A numeric constant. Example: \code{3.45}
  \item \bold{haversineDistance(<numeric>, <numeric>)}: The haversine distance (in meters) to the location specified by the arguments. The first 
         argument defines the longitude of the location. Second argument defines the latitude of the location. 
         The Haversine distance is calculated for every node. Ways and relations are ignored.
         Example: \code{haversineDistance(-0.0014, 51.4778)} if you want to calculate the distance between nodes and the location
         within the Greenwich Park.
}
}

\subsection{numeric_comparison}{
A \dfn{numeric comparison} allows comparing of two numeric expressions.
The operators \code{>}, \code{<}, \code{<=}, \code{>=}, \code{==} are supported.
This allows filtering of locations by distance. E.g. \code{haversineDistance(-0.0014, 51.4778) < 500} would drop all nodes
further away than 500m from the specified location. 
Only the remaining nodes and all ways and relations are passed to the function callbacks.
}

\subsection{filter_expression}{
A \dfn{filter expression} consists of:
\itemize{
  \item \bold{<filter_expression> & <filter_expression>}: logical and (\code{&} and \code{&&} have identical meaning)
  \item \bold{<filter_expression> | <filter_expression>}: logical or (\code{|} and \code{||} have identical meaning)
  \item \bold{!<filter_expression>}: Negation of a filter expression.
  \item \bold{(<filter_expression>)}: A filter expression surrounded by parentheses. Signify precedence of operators.
  \item \bold{<numeric_comparison>}: A numeric comparison as described above.
  \item \bold{id(<string>, <entity_type>)}: Allows filtering by object id. Since identities are not unique among 
        OSM objects, you have to provide the entity type. E.g. \code{id("268533448", EntityBits.way)} in order
        to get the Royal Observatory in Greenwich.
  \item \bold{value == <string>}: Get OSM objects containing a tag with the value specified by the string.
        E.g. \code{value == "residential"} or the short version \code{v == "residential"}. 
  \item \bold{key == <string>}: Get OSM object containing a tag with the key specified by the string.
        E.g. \code{key == "highway"} or the short version \code{k == "highway"}.
  \item \bold{value \%grepl\% <string>}: Get OSM object containing a tag with a value matching the regular expression
        specified by the string (same for key). Implementation of ECMAScript Regex is used.
        E.g. \code{k \%grepl\% "^addr:[a-z]+"}
  \item \bold{value \%contains\% <string>}: Equivalent to \code{value \%grepl\% ".*<string>.*"}
  \item \bold{tag(<string>,<string>)}: Get OSM object containing a tag with a key specified by the first argument and
        a value specified by the second argument. E.g. \code{tag("highway","residential")} or the short version
        \code{t("highway","residential")}. \emph{Note:} This is not equivalent to the expression
        \code{k == "highway" & v == "residential"}. In the latter case an Object would fulfill the condition if
        the OSM Object has \emph{one} tag with the key \code{"highway"} and \emph{another} tag with the value \code{"residential"}.
        The \code{tag} keyword drops all objects not containing the specified key-value pair in \emph{one} tag.
  \item \bold{boundingBox(<numeric>,<numeric>,<numeric>,<numeric>)}: Filtering OSM objects within a bounding box, specified
        by min longitude, min latitude, max longitude and max latitude. A way is considered part of the bounding box if
        at least one node reference lies within the bounding box. A relation is considered part of the bounding box
        if at least one member lies within the bounding box. However, if a relation is a super-relation of a relation
        within the bounding box and the sub-relation is defined after its parent 
        (and the super-relation has no other members within the bounding box), 
        the super-relation is not passed to the \R side. I don't know if this issue is relevant in practice. 
}
} 
}

\value{
  \code{object_filter} returns an object of class \code{ObjectFilter} (reference class).
}

\references{
%% ~put references to the literature/web site here ~
}

\author{
Lukas Huwiler \email{lukas.huwiler@gmx.ch}
}
\note{
}

\seealso{
\code{\link[Rosmium]{osm_apply}}
\code{\link[Rosmium]{osm_write}}
}

\examples{
example_file <- system.file("osm_example/bern_switzerland.osm.pbf", package = "Rosmium")
reader <- new(Reader, example_file, EntityBits.nwr)

# Get the institute of statistics in Berne
university_filter <- object_filter(tag("amenity", "university") & v == "Institut Statistik")
func <- function(x) x
university <- osm_apply(reader, node_func = func, way_func = func, rel_func = func, filter = university_filter) 

# Now find all pubs within a radius of 500 meters
loc <- university[[1]]$location
lon <- format(loc["lon"], nsmall = 7)
lat <- format(loc["lat"], nsmall = 7)
bar_filter <- object_filter(paste0("t('amenity','pub') & haversineDistance(", lon, ",", lat, ") <= 500"), is_char = TRUE)
bars <- osm_apply(reader, node_func = func, way_func = func, rel_func = func, filter = bar_filter)
}
