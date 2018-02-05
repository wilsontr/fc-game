<?xml version="1.0" encoding="UTF-8"?>
<tileset name="map-meta-tiles" tilewidth="8" tileheight="8" tilecount="32" columns="4">
 <image source="map-meta-tiles.bmp" width="32" height="64"/>
 <tile id="0" type="no collide"/>
 <tile id="1" type="all collide"/>
 <tile id="2" type="enemy only collide"/>
 <tile id="3" type="player start">
  <properties>
   <property name="test" value="1"/>
  </properties>
 </tile>
 <tile id="4" type="enemy start"/>
 <tile id="5" type="ladder"/>
</tileset>
