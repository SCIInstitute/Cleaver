uniform sampler2D texture0;
uniform sampler2D texture1;

uniform vec2 camerarange;
uniform vec2 screensize;

varying vec2 texCoord;


float readDepth( in vec2 coord ) {
        return (2.0 * camerarange.x) / (camerarange.y + camerarange.x - texture2D( texture0, coord ).x * (camerarange.y - camerarange.x));
}

float compareDepths( in float depth1, in float depth2 ) {
        float aoCap = 1.0;
        float aoMultiplier=10000.0;
        float depthTolerance=0.000;
        float aorange = 10.0;// units in space the AO effect extends to (this gets divided by the camera far range
        float diff = sqrt( clamp(1.0-(depth1-depth2) / (aorange/(camerarange.y-camerarange.x)),0.0,1.0) );
        float ao = min(aoCap,max(0.0,depth1-depth2-depthTolerance) * aoMultiplier) * diff;
        return ao;
}

void main(void)
{
        float depth = readDepth( texCoord );
        float d;

        float pw = 1.0 / screensize.x;
        float ph = 1.0 / screensize.y;

        float aoCap = 1.0;

        float ao = 0.0;

        float aoMultiplier=10000.0;

        float depthTolerance = 0.001;

        float aoscale=1.0;

        d=readDepth( vec2(texCoord.x+pw,texCoord.y+ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x-pw,texCoord.y+ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x+pw,texCoord.y-ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x-pw,texCoord.y-ph));
        ao+=compareDepths(depth,d)/aoscale;

        pw*=2.0;
        ph*=2.0;
        aoMultiplier/=2.0;
        aoscale*=1.2;

        d=readDepth( vec2(texCoord.x+pw,texCoord.y+ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x-pw,texCoord.y+ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x+pw,texCoord.y-ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x-pw,texCoord.y-ph));
        ao+=compareDepths(depth,d)/aoscale;

        pw*=2.0;
        ph*=2.0;
        aoMultiplier/=2.0;
        aoscale*=1.2;

        d=readDepth( vec2(texCoord.x+pw,texCoord.y+ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x-pw,texCoord.y+ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x+pw,texCoord.y-ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x-pw,texCoord.y-ph));
        ao+=compareDepths(depth,d)/aoscale;

        pw*=2.0;
        ph*=2.0;
        aoMultiplier/=2.0;
        aoscale*=1.2;

        d=readDepth( vec2(texCoord.x+pw,texCoord.y+ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x-pw,texCoord.y+ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x+pw,texCoord.y-ph));
        ao+=compareDepths(depth,d)/aoscale;

        d=readDepth( vec2(texCoord.x-pw,texCoord.y-ph));
        ao+=compareDepths(depth,d)/aoscale;

        ao/=16.0;

        gl_FragColor = vec4(1.0-ao) * texture2D(texture1,texCoord);
}
