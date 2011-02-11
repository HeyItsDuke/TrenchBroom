//
//  FaceOffsetTool.m
//  TrenchBroom
//
//  Created by Kristian Duske on 05.02.11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "FaceOffsetTool.h"
#import "Face.h"
#import "Vector2f.h"
#import "Vector3f.h"
#import "Math.h"
#import "SelectionManager.h"
#import "Ray3D.h"
#import "HalfSpace3D.h"
#import "Plane3D.h"

static NSArray* smallCircle;
static NSArray* largeCircle;
static NSArray* ring;

@implementation FaceOffsetTool

+ (void)initialize {
    smallCircle = [makeCircle(3, 20) retain];
    largeCircle = [makeCircle(10, 32) retain];
    ring = [makeRing(10, 11, 32) retain];
}

- (id)initWithFace:(Face *)theFace selectionManager:(SelectionManager *)theSelectionManager {
    if (theFace == nil)
        [NSException raise:NSInvalidArgumentException format:@"face must not be nil"];
    if (theSelectionManager == nil)
        [NSException raise:NSInvalidArgumentException format:@"selection manager must not be nil"];
    
    if (self = [self init]) {
        face = [theFace retain];
        selectionManager = [theSelectionManager retain];
        lastSurfacePos = [[Vector3f alloc] init];
    }
    
    return self;
}

- (void)renderPoints:(NSArray *)points z:(float)z {
    NSEnumerator* pointEn = [points objectEnumerator];
    Vector2f* point2D;
    Vector3f* point3D = [[Vector3f alloc] init];
    [point3D setZ:z]; // slight offset from face
    
    while ((point2D = [pointEn nextObject])) {
        [point3D setX:[point2D x]];
        [point3D setY:[point2D y]];
        Vector3f* w = [face worldCoordsOf:point3D];
        glVertex3f([w x], [w y], [w z]);
    }
}

- (void)render {
    glPolygonMode(GL_FRONT, GL_FILL);
    
    glColor4f(0.02, 0.5, 0.9, 0.65);
    glBegin(GL_TRIANGLE_STRIP);
    [self renderPoints:ring z:0.01];
    glEnd();
    
    glColor4f(1, 1, 1, 0.65);
    glBegin(GL_POLYGON);
    [self renderPoints:largeCircle z:0.01];
    glEnd();
    
    glColor4f(0.02, 0.5, 0.9, 0.65);
    glBegin(GL_POLYGON);
    [self renderPoints:smallCircle z:0.02];
    glEnd();
    
    Vector3f* v = [[Vector3f alloc] init];
    [v setZ:0.02];
    Vector3f* w;
    
    glBegin(GL_TRIANGLES);
    // top arrow
    [v setX:-2];
    [v setY:4];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);
    
    [v setX:0];
    [v setY:9];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);
    
    [v setX:2];
    [v setY:4];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);
    
    // right arrow
    [v setX:4];
    [v setY:2];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);
    
    [v setX:9];
    [v setY:0];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);

    [v setX:4];
    [v setY:-2];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);

    // bottom arrow
    [v setX:2];
    [v setY:-4];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);
    
    [v setX:0];
    [v setY:-9];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);
    
    [v setX:-2];
    [v setY:-4];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);
    
    // left arrow
    [v setX:-4];
    [v setY:-2];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);
    
    [v setX:-9];
    [v setY:0];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);
    
    [v setX:-4];
    [v setY:2];
    w = [face worldCoordsOf:v];
    glVertex3f([w x], [w y], [w z]);

    glEnd();
}

- (BOOL)hitByRay:(Ray3D *)theRay {
    if (theRay == nil)
        [NSException raise:NSInvalidArgumentException format:@"ray must not be nil"];
    
    Plane3D* plane = [[face halfSpace] boundary];
    Vector3f* is = [plane intersectWithRay:theRay];
    if (is == nil)
        return NO;
    
    [is sub:[face center]];
    return flte([is lengthSquared], 11 * 11);
}

- (void)startDrag:(Ray3D *)theRay {
    Plane3D* plane = [[face halfSpace] boundary];
    Vector3f* is = [plane intersectWithRay:theRay];
    if (is == nil)
        return;
    
    [lastSurfacePos setFloat:[face surfaceCoordsOf:is]];
}

- (void)drag:(Ray3D *)theRay {
    Plane3D* plane = [[face halfSpace] boundary];
    Vector3f* is = [plane intersectWithRay:theRay];
    if (is == nil)
        return;
    
    Vector3f* surfacePos = [face surfaceCoordsOf:is];
    int dx = (int)[surfacePos x] - (int)[lastSurfacePos x];
    int dy = (int)[surfacePos y] - (int)[lastSurfacePos y];

    if (dx != 0) {
        [face setXOffset:[face xOffset] + dx];
        [lastSurfacePos setX:[surfacePos x]];
    }
    
    if (dy != 0) {
        [face setYOffset:[face yOffset] + dy];
        [lastSurfacePos setY:[surfacePos y]];
    }
}

- (void)endDrag:(Ray3D *)theRay {
}

- (void)dealloc {
    [lastSurfacePos release];
    [face release];
    [selectionManager release];
    [super dealloc];
}

@end
