void sphereCast(vec3 origin, vec3 ray, vec4 sphere, out vec3 start, out vec3 end)
{
    vec3 p = origin - sphere.xyz;
    
    float 
        r = length(p),
        d = length(cross(p, ray));    //nearest distance from view-ray to the center
        
    if (d >= sphere.w+fix) {
        start = vec3(0.0);
        end = vec3(0.0);
        return;    //early exit
    }
        
    float
        sr = sqrt(sphere.w*sphere.w - d*d),    //radius of slicing circle
        dr = -dot(p, ray);    //distance from camera to the nearest point\
    
    if(r>sphere.w && dr>0){
        start = origin + ray * (dr-sr);
        end = origin + ray * (dr+sr);
    }else if(r<=sphere.w){
        start = origin;
        end = origin + ray * (dr+sr);
    }else{
        start = vec3(0.0);
        end = vec3(0.0);
    }
}

void rayDistance(vec3 posCam, vec3 ray, out vec3 start, out vec3 end){
    vec3 startAtm, endAtm;
    vec3 startGnd, endGnd;
    sphereCast(posCam, ray, atmosphereSphere, startAtm, endAtm);
    sphereCast(posCam, ray, groundSphere, startGnd, endGnd);

    if (startAtm==posCam){
        start = posCam;
        if (startGnd==vec3(0.0))
            end = endAtm;
        else
            end = startGnd;
    }
    else{
        start = startAtm;
        if (startGnd==vec3(0.0))
            end = endAtm;
        else
            end = startGnd;
    }
}