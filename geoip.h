#ifndef SC6_GEOIP_H
#define SC6_GEOIP_H

struct geoip {
    unsigned int startIp;
    unsigned int endIp;
    float lat;
    float lon;
};

struct geoip *initGeoip();

void freeGeoip(struct geoip *geoip);

void loadGeoip(struct geoip *geoip);

struct geoip *findGeoip(struct geoip *geoip, unsigned int ip);

#endif //SC6_GEOIP_H
