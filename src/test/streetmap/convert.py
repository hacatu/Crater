"""
This script converts OpenStreetMap xml data into csv data including only the length
of streets, positions of intersections, and names of streets.

OpenStreetMap data is organized into nodes, ways, and relations.  Each has its own
id which is unique among its category.  That is, no two nodes have the same id, but
a node and way may have the same id.

Nodes represent points on the earth.  Each has an id, latitude, and longitude.
Other properties, including name, elevation, street sign at location, available
amenities, etc, may be present.  We only care about the id, latitude, longitude,
and name though.  The first three are attributes of the node tag, while name
is specified as a child tag tag.

Tag tags have a "k" and "v" attribute, representing an arbitrary key and value.
(That this is already possible with xml nodes was probably lost on the format developers,
as it is on anyone who chooses xml over json).  Thousands and thousands of
distinct keys exist in the OSM data set, but we only ever care about "name"
and "highway".

Ways represent literally any continuous sequence of line segments, be it open with
distinct ends or closed forming a loop.  This includes areas like parks, tons of
random stuff, and critically for us, roads.  Each way has an id, some number of
nodes constituting it, and possibly tags.  We only care about the "name" and "highway"
tags.  The "highway" tag describes what type of road a way is.  Ways that have it
are roads, but not usually highways, whereas ways that lack it aren't roads.
The nodes constituting a way are denoted by the "ref" attribute of "nd" children of
the way tag.

Relations represent relations between nodes and ways, but we don't care about these.

To find intersections between ways, we have to find nodes that are referenced by multiple
ways.  So we do two passes over the data, extracting
1.a. id, lat, lon, name, and reference count (starting at 0) for each node
  b. id, node list (incrementing reference count for each), and name for each way
2.a. for each way, we remove any nodes with a reference count of 1, combining edges as needed.
     This gives us a new set of data, the edges, where each edge has a starting node id,
	 ending node id, containing way id, and distance.  The distance is obtained by adding together
	 the geodesic distances between consecutive nodes on the way before removing nodes with
	 a reference count of 1.
  b. we remove any nodes with a reference count < 2

Then we can print the results to a csv file.
"""

import xml.etree.ElementTree as ET
import math

def geodist(lat1, lon1, lat2, lon2):
	"""
	Compute geodesic distance between 2 point on the WGS-84 reference ellipsoid for earth
	using Vincenty's formula.  This is accurate to within .5 mm, which is totally necessary.
	https://en.wikipedia.org/wiki/Vincenty%27s_formulae
	"""
	a = 6378137.0 # semimajor axis of earth
	b = 6356752.314245 # semiminor axis of earth
	f = 1 - b/a # "flattening" parameter of earth
	u1 = math.atan((1-f)*lat1) # "reduced latitude" of 1st point
	u2 = math.atan((1-f)*lat2) # "reduced latitude" of 2nd point
	L = lon2 - lon1
	# sin and cos of u1 and u2
	su1 = math.sin(u1)
	cu1 = math.cos(u1)
	su2 = math.sin(u2)
	cu2 = math.cos(u2)
	l = L # lambda, the longitudinal difference on the reference sphere
	while True:
		# sin and cos of l(ambda) and s(igma)
		sl = math.sin(l)
		cl = math.cos(l)
		ss = math.hypot(cu2*sl, cu1*su2 - su1*cu2*cl)
		cs = su1*su2 + cu1*cu2*cl
		s = math.atan2(ss, cs) # sigma, the angle between the points on ref. sphere
		sa = cu1*cu2*sl/ss # sin of alpha, the angle between the extended geodesic and the equator
		ca2 = 1 - sa**2 # cos squared of alpha
		c2sm = cs - 2*su1*su2/ca2 # cos of 2 sigma_m, the latitude of the midpoint of the points
		C = f/16*ca2*(4+f*(4 - 3*ca2))
		l_old = l
		l = L + (1-C)*f*sa*(s + C*sa*(c2sm + C*cs*(-1 + 2*c2sm**2)))
		if abs(l - l_old) < 6e-5:
			break
	t2 = ca2*(a**2 - b**2)/b**2
	A = 1 + t2/16384*(4096+t2*(-768 + t2*(320 - 175*t2)))
	B = t2/1024*(256 + t2*(-128 + t2*(74 - 47*t2)))
	ds = B*ss*(c2sm + B/4*(cs*(-1 + 2*c2sm**2) - B/6*c2sm*(-3 + 4*c2sm**2)))
	return b*A*(s - ds)

with open("resources/rutgers.osm", "r") as f:
	xml = ET.parse(f)

nodes = {}
ways = {}
edges = {}
way_names = {}

r = xml.getroot()
for e in r:
	if e.tag == "node":
		name = e.find("tag[@k='name']")
		name = name.attrib["v"] if name is not None else ""
		nodes[int(e.attrib["id"])] = [float(e.attrib["lat"]), float(e.attrib["lon"]), name, 0]
	elif e.tag == "way" and e.find("tag[@k='highway']") is not None:
		way_id = int(e.attrib["id"])
		name = e.find("tag[@k='name']")
		name = name.attrib["v"] if name is not None else ""
		nds = []
		for nd in e.iterfind("nd"):
			if nds:
				u_id = int(nds[-1].attrib["ref"])
				v_id = int(nd.attrib["ref"])
				nodes[u_id][3] += 1
				nodes[v_id][3] += 1
				nds[-1] = u_id
			nds.append(nd)
		nds[-1] = int(nds[-1].attrib["ref"])
		ways[way_id] = nds
		if name != "":
			way_names[way_id] = name

del r

for way_id, nds in ways.items():
	if len(nds) < 2:
		continue
	x_id = nds[0]
	dist = 0
	lat1, lon1 = nodes[x_id][:2]
	for v_id in nds[1:-1]:
		lat2, lon2, _, refs = nodes[v_id]
		dist += geodist(lat1, lon1, lat2, lon2)
		lat1, lon1 = lat2, lon2
		if refs > 1:
			u, v = x_id, v_id
			if u > v:
				u, v = v, u
			edges[(u, v)] = (way_id, dist)
			x_id = v_id
			dist = 0
	v_id = nds[-1]
	lat2, lon2 = nodes[v_id][:2]
	dist += geodist(lat1, lon1, lat2, lon2)
	u, v = x_id, v_id
	if u > v:
		u, v = v, u
	edges[(u, v)] = (way_id, dist)

del ways

with open("resources/rutgers_roads.csv", "w") as f:
	print("type,id,lat/u,lon/v,-/dist,name", file=f)
	for node_id, (lat, lon, name, refs) in nodes.items():
		if refs > 1:
			print("node,{},{},{},,{}".format(node_id, lat, lon, name), file=f)
	for (u_id, v_id), (way_id, dist) in edges.items():
		print("edge,{},{},{},{},{}".format(way_id, u_id, v_id, dist, way_names.get(way_id, "")), file=f)

