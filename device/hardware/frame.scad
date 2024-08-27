
$fn=25;


powerSupplyWidth = 101;
powerSupplyDepth = 99;
powerSupplyHeight = 36;




nutBoxHeight = 15;
screwRange = 1.5;
screwLength = 40;
nutRange = 4;
nutHeight = 3.5;

boxMargin = 1;

wallThickness = 1.5;
bottomThickness = 1.5;

widthMargin = 16;
antiPullGuiderWidth = 25;
antiPullOpeningY = 10;
antiPullOpeningHeight = 7;



cutHeight = antiPullOpeningY + antiPullOpeningHeight + bottomThickness;


// Bottom Part
difference() {
    box();
    translate([powerSupplyWidth + widthMargin + boxMargin * 2 + wallThickness, 0, cutHeight])
    cube([500, 500, 500]);
}


// Top part
translate([0, 100, 50])
{
    intersection() {
        box();
        translate([powerSupplyWidth + widthMargin + boxMargin * 2 + wallThickness, 0, cutHeight])
        cube([500, 500, 500]);
    }
    
    translate([0, 0, powerSupplyHeight + boxMargin * 2 + bottomThickness])
    {
        difference() {
            cube([
                powerSupplyWidth + widthMargin + antiPullGuiderWidth + boxMargin * 2 + wallThickness, 
                powerSupplyDepth + (boxMargin + wallThickness) * 2,
                bottomThickness
            ]);
              translate([powerSupplyWidth + widthMargin + antiPullGuiderWidth + boxMargin * 2 + wallThickness - 6, 
            powerSupplyDepth + (boxMargin + wallThickness) * 2 - 6, 
            0]) {
                cylinder(r=screwRange, h=bottomThickness);
                translate([0, -92, 0])
                cylinder(r=screwRange, h=bottomThickness);
            }
        }

        translate([-16, powerSupplyDepth/2 + (boxMargin + wallThickness) - 16 /2, 0])
        difference() {
            cube([
                16,
                16,
                bottomThickness
            ]);
            translate([8, 8, 0])
            cylinder(h=bottomThickness, r=screwRange);
        }
        
        holderWidth = 15;
        translate([powerSupplyWidth + boxMargin * 2 + wallThickness, wallThickness + 5, bottomThickness - 10]) {
            cube([2, holderWidth, 10]);
            translate([0, powerSupplyDepth + boxMargin * 2 - 5 * 2 - holderWidth, 0])
            cube([2, holderWidth, 10]);
        }
    }
}


module box() {

    difference() {
        cube([
            powerSupplyWidth + widthMargin + antiPullGuiderWidth + boxMargin * 2 + wallThickness, 
            powerSupplyDepth + (boxMargin + wallThickness) * 2,
            powerSupplyHeight + boxMargin * 2 + bottomThickness, 
        ]);
        translate([wallThickness, wallThickness, bottomThickness]) {
            cube([
                powerSupplyWidth + widthMargin + antiPullGuiderWidth + (boxMargin) * 2, 
                powerSupplyDepth + (boxMargin) * 2,
                powerSupplyHeight + boxMargin * 2, 
            ]);
        }
        
        translate([powerSupplyWidth + widthMargin + antiPullGuiderWidth + boxMargin * 2 + wallThickness - 6, 
        powerSupplyDepth + (boxMargin + wallThickness) * 2 - 6, 
        0]) {
            cylinder(r=screwRange, h=bottomThickness);
            translate([0, -92, 0])
            cylinder(r=screwRange, h=bottomThickness);
        }
    }


    // Connection points
    translate([
        0, 
        powerSupplyDepth / 2 + (boxMargin + wallThickness) + 16/2, 
        powerSupplyHeight + boxMargin * 2 + bottomThickness - nutBoxHeight])
    {
        rotate([0, 0, 180])
        connectionPoint(screwRange, screwLength, nutRange, nutHeight, nutBoxHeight);
    }


    translate([
        powerSupplyWidth + widthMargin + antiPullGuiderWidth + boxMargin * 2 + wallThickness, 
        powerSupplyDepth + (boxMargin + wallThickness) * 2, 
        bottomThickness])
    {
        rotate([0, 0, 180])
        {
            
            connectionPoint(screwRange, screwLength, nutRange, nutHeight, cutHeight - bottomThickness + 3);
            translate([0, 92, 0])
            connectionPoint(screwRange, screwLength, nutRange, nutHeight, cutHeight - bottomThickness + 3);
        }
    }




    translate([powerSupplyWidth + boxMargin * 2 + wallThickness, wallThickness, bottomThickness]) {
        cube([2, powerSupplyDepth + boxMargin * 2, 10]);
        
      
        translate([widthMargin, 0, 0]) {
            translate([0, 8, 0])            
            antiPullGuider(antiPullGuiderWidth, 25, powerSupplyHeight + 2 * boxMargin, 7, antiPullOpeningHeight, antiPullOpeningY);
            translate([0, 45, 0])
            antiPullGuider(antiPullGuiderWidth, 25, powerSupplyHeight + 2 * boxMargin, 4, antiPullOpeningHeight, antiPullOpeningY);


            translate([antiPullGuiderWidth - wallThickness, 0, 0]) {
                cube([wallThickness, 10, powerSupplyHeight + boxMargin * 2]);
                translate([0, 30, 0])
                cube([wallThickness, 15, powerSupplyHeight + boxMargin * 2]);
                translate([0, 25 + 40, 0])
                cube([wallThickness, powerSupplyDepth + boxMargin * 2 - (25 + 40),      powerSupplyHeight + boxMargin * 2]);
            }
            

        }
    }
}




module connectionPoint(screwRange, screwLength, nutRange, nutHeight, height) {
    padding = 3 + 1.5;
    width = screwRange * 2 + padding * 2;
    depth = screwRange * 2 + padding * 2;
    
    difference() {
        cube([width, depth, height]);
        
        translate([width / 2, depth / 2, 0])
        {
            translate([0, 0, height - screwLength])
            cylinder(r=screwRange, h=screwLength);

            translate([-nutRange, -nutRange, height / 2 - nutHeight / 2])        
            cube([width, nutRange * 2, nutHeight]);
        }
    }
}

module antiPullGuider(width, depth, height, pathThickness, pathHeight, holeOffsetY) {
    bendDepth = 10;
    bendWidth = 15;
    xPadding = (width - bendWidth) / 2;
    yPadding = (depth - bendDepth) / 2;
    
    difference() {
        cube([width, depth, height]);
        
        
        translate([0, yPadding - pathThickness / 2, holeOffsetY]) {
            cube([xPadding, pathThickness, pathHeight]);
            translate([xPadding, 0, 0]) {
                cube([pathThickness, bendDepth, pathHeight]);
                translate([0, bendDepth, 0]) {
                    cube([bendWidth, pathThickness, pathHeight]);
                     
                    translate([bendWidth - pathThickness, -bendDepth, 0])
                    cube([pathThickness, bendDepth, pathHeight]);
                }   
                
            }
            translate([width - xPadding, 0, 0]) {
                cube([xPadding, pathThickness, pathHeight]);
            }
        }
    }
}
