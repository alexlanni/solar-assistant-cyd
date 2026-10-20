// =====================================================================
//  CYD Solar Monitor - contenitore per ESP32-2432S028 "Cheap Yellow Display"
//  OpenSCAD (testato con 2021.01), completamente parametrico.
//
//  Parti (selezionare con  -D 'part="..."'):
//    "front"     cornice frontale (stampa: faccia in giu' sul piatto)
//    "back"      guscio posteriore (stampa: fondo sul piatto)
//    "stand"     supporto da tavolo inclinato (stampa: base sul piatto)
//    "diffuser"  dischetto diffusore LED (PETG/PLA naturale/trasparente)
//    "assembly"  vista assemblata (solo anteprima)
//    "exploded"  vista esplosa (solo anteprima)
//    "onstand"   contenitore appoggiato sul supporto (solo anteprima)
//    "board"     modello semplificato della scheda (solo anteprima)
//
//  Sistema di riferimento (vista frontale, display orizzontale come con
//  tft.setRotation(1), USB a DESTRA, slot microSD in BASSO, LDR a sinistra):
//    origine = centro del PCB, X verso destra, Y verso l'alto,
//    Z uscente dallo schermo; z=0 = faccia POSTERIORE del PCB.
//
//  Fonti delle quote: vedi README.md. I valori marcati [MISURA] sono
//  ricavati da disegni/modelli online e vanno verificati col calibro.
// =====================================================================

part = "assembly";

/* ---------------- Tolleranze / stampa ---------------- */
clr       = 0.2;    // gioco generico FDM
pcb_clr   = 0.4;    // gioco tra bordo PCB/display e parete interna (display 50.0 +-0.2)
wall      = 2.0;    // spessore pareti laterali
floor_t   = 2.0;    // spessore fondo guscio posteriore
r_out     = 3.0;    // raggio spigoli esterni
$fn       = 48;

/* ---------------- Scheda (PCB) ---------------- */
pcb_l     = 86.0;   // lunghezza PCB (datasheet: 50.0 x 86.0)
pcb_w     = 50.0;   // altezza PCB
pcb_t     = 1.6;    // spessore PCB [MISURA]
hole_dx   = 78.0;   // interasse fori in X (4 mm dai bordi)
hole_dy   = 42.0;   // interasse fori in Y
hole_d    = 3.2;    // foro PCB (R1.60 da disegno ufficiale)

/* ---------------- Display (modulo JC2432A028N) ---------------- */
disp_l    = 69.2;   // lunghezza retroilluminazione (BL)
disp_w    = 50.0;   // larghezza BL
disp_cx   = 0.0;    // centro display rispetto al centro PCB [MISURA]
disp_h    = 3.3;    // altezza vetro sopra la faccia FRONTALE del PCB [MISURA!]
aa_l      = 57.6;   // area attiva
aa_w      = 43.2;
aa_off_x  = 3.05;   // distanza area attiva dal bordo SINISTRO (lato LDR, opposto al flat) del display
aa_cy     = 0.0;    // area attiva centrata in Y (3.40 per lato)
win_margin= 1.0;    // allargamento della finestra oltre l'area attiva (per lato)
disp_gap  = 0.4;    // aria tra vetro e cornice (il touch resistivo non deve essere premuto)
lip_t     = 1.2;    // spessore della cornice sopra il vetro (sottile -> touch comodo ai bordi)

/* ---------------- Componenti ---------------- */
// LDR (fotoresistenza) sul FRONTE, a sinistra del display [MISURA]
ldr_x = -39.5;  ldr_y = -14.0;  ldr_sx = 3.0; ldr_sy = 6.0;
ldr_hole_d = 3.0;
ldr_hole   = true;
// LED RGB sul RETRO (IO4/IO16/IO17) [MISURA - da verificare sulla propria scheda]
led_x = -12.5;  led_y = -14.0;  led_h = 1.3;
led_window_mode = "insert";   // "insert" = foro + diffusore separato, "membrane" = membrana integrata, "open"
led_window_d    = 7.0;        // foro di uscita luce sul retro
led_diff_d      = 9.0;        // sede del diffusore (lato interno)
led_diff_t      = 0.7;        // spessore diffusore (2 layer da 0.35 o 3-4 da 0.2)
led_membrane_t  = 0.6;        // spessore membrana se "membrane"
led_pipe        = true;       // tubetto anti-dispersione tra fondo e PCB
led_pipe_wall   = 0.8;
led_pipe_gap    = 1.0;        // distanza tra cima del tubetto e PCB (lascia spazio alle resistenze SMD)
led_halo_groove = true;       // canalino sul retro dal LED al bordo inferiore (alone a parete)
led_groove_w    = 4.0;
led_groove_d    = 1.0;
// Pulsanti RST / BOOT sul retro [MISURA]
btn_rst = [-33.65, 20.2];
btn_boot= [-29.65, 20.2];
btn_tip_z = -4.6;             // punta dei pulsanti (z negativo = sotto il PCB)
pin_hole_d = 1.8;             // foro per graffetta
// micro-USB sul bordo destro, sul retro del PCB [MISURA]
usb_y  = -9.3;   usb_zc = -1.25;  usb_protr = 0.6;
usb_open_w = 12.0;  usb_open_h = 8.0;   // apertura per la spina (solo se bay_len=0)
usb_dual   = false;             // variante a 2 porte (USB-C + micro) [posizione USB-C NON verificata]
usbc_y     = 4.0;   usbc_open_w = 13.0;  usbc_open_h = 7.5;
// slot microSD sul bordo inferiore [MISURA]
sd_x0 = -2.6;  sd_x1 = 12.4;  sd_cutout = true;
sd_cut_z0 = -2.8; sd_cut_z1 = 0.4;
// componente piu' alto sul retro (pulsanti / JST) e spazio libero dietro al PCB
rear_clear = 7.0;

/* ---------------- Vano cavo USB ---------------- */
bay_len    = 30.0;   // vano interno a destra per spina+curva del cavo (0 = spina che esce dal fianco)
usb_plug_len = 16.0; // lunghezza parte rigida della spina dal bordo PCB
cable_d    = 4.0;    // diametro cavo USB
cable_exit = "bottom"; // "bottom" (passacavo sul fondo, con fascetta) o "back" (foro nel retro)

/* ---------------- Viti ---------------- */
screw_len   = 12;    // vite M3 a testa bombata/cilindrica bassa (ISO 7380 / DIN 7985), dal retro
screw_hole  = 3.4;   // passaggio M3
screw_pilot = 2.5;   // preforo per autofilettatura M3 nella plastica (2.6-2.8 se PETG duro)
head_d      = 6.4;   // svaso per la testa
standoff_d  = 5.5;
post_d      = 5.5;

/* ---------------- Asole a parete ---------------- */
key_dx   = 28;       // distanza delle asole dal centro del contenitore
key_y    = 2.5;      // centro del foro grande (la vite poi scorre nell'asola verso l'alto)
key_big  = 8.5;      // foro di ingresso testa vite (testa max ~8 mm)
key_slot = 4.2;      // larghezza asola (vite da 3.5-4 mm)
key_len  = 8.0;      // corsa dell'asola
key_fence_h = 3.0;   // altezza recinto interno della testa

/* ---------------- Ventilazione ---------------- */
vent_w = 1.6;  vent_l = 9.0;  vent_pitch = 4.0;

/* ---------------- Supporto da tavolo ---------------- */
stand_angle = 18;    // inclinazione all'indietro (gradi dalla verticale)
stand_w     = 80;
stand_back  = 40;    // lunghezza dello schienale
stand_thick = 4;
stand_base_t= 3;
stand_lip_h = 4;
stand_lip_t = 2.5;
stand_pad_w = 12;    // appoggi pieni alle estremita' della mensola (il centro e' ribassato per le fessure)
stand_nubs  = true;  // pioli che entrano nei fori delle asole (bloccano lo scorrimento laterale)

/* ================= Quote derivate ================= */
z_pcb_bot  = 0;
z_pcb_top  = pcb_t;
z_glass    = z_pcb_top + disp_h;
z_bez_under= z_glass + disp_gap;
z_top      = z_bez_under + lip_t;
z_floor_in = -rear_clear;
z_bot      = z_floor_in - floor_t;
z_split    = z_pcb_top;
rab_h      = 1.2;
joint_gap  = 0.2;

in_x0 = -pcb_l/2 - pcb_clr;
in_x1 = (bay_len > 0) ? pcb_l/2 + pcb_clr + bay_len : pcb_l/2 + pcb_clr;
in_y0 = -pcb_w/2 - pcb_clr;
in_y1 =  pcb_w/2 + pcb_clr;
out_x0 = in_x0 - wall; out_x1 = in_x1 + wall;
out_y0 = in_y0 - wall; out_y1 = in_y1 + wall;
xc = (in_x0 + in_x1)/2;
box_T = z_top - z_bot;

holes = [for (sx=[-1,1], sy=[-1,1]) [sx*hole_dx/2, sy*hole_dy/2]];
tip_z  = z_top - 0.8;
seat_z = tip_z - screw_len;
bay_boss_d = 7;
bay_bosses = (bay_len > 0) ? [[in_x1 - 3.6, in_y1 - 3.6], [in_x1 - 3.6, in_y0 + 3.6]] : [];
exit_x = min(pcb_l/2 + usb_protr + usb_plug_len + cable_d/2 + 1,
             in_x1 - 3.6 - bay_boss_d/2 - cable_d/2 - 0.5);
keys = [[xc - key_dx, key_y], [xc + key_dx, key_y]];
aa_cx = disp_cx - disp_l/2 + aa_off_x + aa_l/2;
win_l = aa_l + 2*win_margin;
win_w = aa_w + 2*win_margin;
cable_zc = z_split - cable_d/2 - 0.3;

echo(str("CYD box outer: ", out_x1-out_x0, " x ", out_y1-out_y0, " x ", box_T, " mm"));
echo(str("screw head seat z=", seat_z, " (fondo esterno z=", z_bot, ")  exit_x=", exit_x));
assert(seat_z > z_bot + 1.6, "screw_len troppo corta/lunga: la testa non e' incassata");
assert(seat_z < btn_tip_z - 1.0, "screw_len troppo corta: torretta troppo alta");

/* ================= Utilita' ================= */
module rr2d(x0, x1, y0, y1, r) {
    rr = max(0.01, min(r, (x1-x0)/2 - 0.01, (y1-y0)/2 - 0.01));
    translate([x0+rr, y0+rr]) offset(r=rr) square([x1-x0-2*rr, y1-y0-2*rr]);
}
module outline(o=0) { rr2d(out_x0-o, out_x1+o, out_y0-o, out_y1+o, r_out+o); }
module zslab(z0, z1) { translate([0,0,z0]) linear_extrude(z1-z0) children(); }
module ring(o_out, o_in) { difference(){ outline(o_out); outline(o_in); } }
module stadium(p0, p1, d) { hull(){ translate(p0) circle(d=d); translate(p1) circle(d=d); } }

/* ================= Guscio posteriore ================= */
module back_shell() {
    difference() {
        union() {
            zslab(z_bot, z_floor_in) outline(0);                         // fondo
            zslab(z_bot, z_split) ring(0, -wall);                        // pareti
            zslab(z_split, z_split + rab_h) ring(0, -wall/2);            // battuta (meta' esterna)
            // torrette PCB
            for (h = holes) translate([h[0], h[1], 0]) {
                zslab(z_floor_in - 0.01, z_pcb_bot) circle(d=standoff_d);
                zslab(z_floor_in - 0.01, seat_z + 0.7) circle(d=9);
            }
            // torrette del vano cavo
            for (b = bay_bosses) translate([b[0], b[1], 0]) {
                zslab(z_floor_in - 0.01, z_split - 0.1) circle(d=bay_boss_d);
            }
            // guide per la graffetta (RST/BOOT)
            for (b = [btn_rst, btn_boot]) translate([b[0], b[1], 0])
                zslab(z_floor_in - 0.01, btn_tip_z - 0.6) circle(d=pin_hole_d + 1.8);
            // recinto delle asole
            for (k = keys) zslab(z_floor_in - 0.01, z_floor_in + key_fence_h) difference() {
                stadium(k, k + [0, key_len], key_big + 2*1.2);
                stadium(k, k + [0, key_len], key_big);
            }
            // tubetto luce LED
            if (led_pipe) translate([led_x, led_y, 0])
                zslab(z_floor_in - 0.01, -led_pipe_gap) difference() {
                    circle(d = led_diff_d + 0.4 + 2*led_pipe_wall);
                    circle(d = led_diff_d + 0.4);
                }
        }
        // fori viti + svasi testa (PCB e vano)
        for (p = concat(holes, bay_bosses)) translate([p[0], p[1], 0]) {
            zslab(z_bot - 1, z_split + 1) circle(d=screw_hole);
            zslab(z_bot - 1, seat_z) circle(d=head_d);
        }
        // fori graffetta + incisioni R / B
        for (b = [btn_rst, btn_boot]) translate([b[0], b[1], 0]) zslab(z_bot - 1, 0) circle(d=pin_hole_d);
        for (b = [[btn_rst, "R"], [btn_boot, "B"]])
            translate([b[0][0], b[0][1] - 4.2, z_bot - 0.01]) linear_extrude(0.41)
                mirror([1,0,0]) text(b[1], size=2.6, halign="center", valign="center",
                                     font="Liberation Sans:style=Bold");
        // asole: foro grande + asola nel fondo
        for (k = keys) zslab(z_bot - 1, z_floor_in + 0.01) {
            translate(k) circle(d=key_big);
            stadium(k, k + [0, key_len], key_slot);
        }
        // finestra LED
        translate([led_x, led_y, 0]) {
            if (led_window_mode == "insert") {
                zslab(z_bot - 1, z_floor_in + 0.01) circle(d=led_window_d);
                zslab(z_floor_in - led_diff_t - 0.1, z_floor_in + 0.01) circle(d=led_diff_d);
            } else if (led_window_mode == "membrane") {
                zslab(z_bot + led_membrane_t, z_floor_in + 0.01) circle(d=led_window_d);
            } else {
                zslab(z_bot - 1, z_floor_in + 0.01) circle(d=led_window_d);
            }
        }
        // canalino alone LED verso il bordo inferiore
        if (led_halo_groove) {
            g0 = (led_window_mode == "membrane") ? led_y - led_window_d/2 : led_y;
            translate([led_x - led_groove_w/2, out_y0 - 1, z_bot - 1])
                cube([led_groove_w, g0 - out_y0 + 1, led_groove_d + 1]);
        }
        // feritoie sul fondo
        for (x = [-6 : vent_pitch : 30]) zslab(z_bot - 1, z_floor_in + 1)
            stadium([x, 9.5], [x, 9.5 + vent_l], vent_w);
        for (x = [ 0 : vent_pitch : 30]) zslab(z_bot - 1, z_floor_in + 1)
            stadium([x, -21.5], [x, -21.5 + vent_l], vent_w);
        // feritoie sulle pareti superiore e inferiore (convezione a parete)
        for (x = [-20 : vent_pitch : 32]) translate([x - vent_w/2, in_y1 - 1, -6]) cube([vent_w, wall + 2, 5.5]);
        for (x = [ 18 : vent_pitch : 36]) translate([x - vent_w/2, out_y0 - 1, -6]) cube([vent_w, wall + 2, 5.5]);
        // microSD
        if (sd_cutout) translate([sd_x0 - 1, out_y0 - 1, sd_cut_z0]) cube([sd_x1 - sd_x0 + 2, wall + 2, sd_cut_z1 - sd_cut_z0]);
        // uscita USB
        usb_cuts();
        if (bay_len > 0 && cable_exit == "bottom") {
            // passacavo a U nella parete inferiore (stringe leggermente il cavo)
            translate([exit_x, out_y0 - 1, cable_zc]) rotate([-90,0,0]) cylinder(d=cable_d - 0.4, h=wall + 2);
            translate([exit_x - (cable_d - 0.4)/2, out_y0 - 1, cable_zc]) cube([cable_d - 0.4, wall + 2, 10]);
            // ingresso svasato lato esterno
            translate([exit_x, out_y0 - 0.01, cable_zc]) rotate([-90,0,0]) cylinder(d1=cable_d + 1.5, d2=cable_d - 0.4, h=1.0);
            // asole per fascetta (ancoraggio antistrappo)
            for (sx = [-1, 1]) translate([exit_x + sx*(cable_d/2 + 1.5), in_y0 + 11, 0])
                zslab(z_bot - 1, z_floor_in + 1) square([1.6, 3.5], center=true);
        }
        if (bay_len > 0 && cable_exit == "back")
            translate([exit_x - 3, -16, 0]) zslab(z_bot - 1, z_floor_in + 1) rr2d(-6, 6, -4.5, 4.5, 1.5);
    }
}

module usb_cuts() {
    if (bay_len <= 0) {
        translate([in_x1 - 1, usb_y - usb_open_w/2, usb_zc - usb_open_h/2]) cube([wall + 2, usb_open_w, usb_open_h]);
        if (usb_dual)
            translate([in_x1 - 1, usbc_y - usbc_open_w/2, usb_zc - usbc_open_h/2]) cube([wall + 2, usbc_open_w, usbc_open_h]);
    }
}

/* ================= Cornice frontale ================= */
module front_bezel() {
    difference() {
        union() {
            zslab(z_bez_under, z_top) outline(0);                                       // piastra
            zslab(z_split + rab_h + joint_gap, z_bez_under + 0.01) ring(0, -wall);     // pareti
            zslab(z_split + joint_gap, z_split + rab_h + joint_gap + 0.01) ring(-wall/2 - clr, -wall); // battuta interna
            for (h = holes) translate([h[0], h[1], 0]) zslab(z_pcb_top, z_bez_under + 0.01) circle(d=post_d);
            for (b = bay_bosses) translate([b[0], b[1], 0]) zslab(z_split, z_bez_under + 0.01) circle(d=bay_boss_d);
            // schermo luce attorno alla LDR
            if (ldr_hole) translate([ldr_x, ldr_y, 0]) zslab(z_pcb_top + 0.3, z_bez_under + 0.01) difference() {
                square([ldr_sx + 0.6 + 1.6, ldr_sy + 0.6 + 1.6], center=true);
                square([ldr_sx + 0.6, ldr_sy + 0.6], center=true);
            }
            // linguetta che chiude il passacavo
            if (bay_len > 0 && cable_exit == "bottom")
                translate([exit_x - (cable_d - 0.4 - 2*clr)/2, out_y0, z_split - 0.5])
                    cube([cable_d - 0.4 - 2*clr, wall, rab_h + joint_gap + 0.5 + 0.01]);
        }
        // finestra display con smusso a 45 gradi (bordo sottile -> touch usabile fino al bordo)
        hull() {
            translate([aa_cx, aa_cy, z_bez_under - 0.01]) linear_extrude(0.01) square([win_l, win_w], center=true);
            translate([aa_cx, aa_cy, z_top]) linear_extrude(0.02) square([win_l + 2*lip_t, win_w + 2*lip_t], center=true);
        }
        // fori pilota per le viti
        for (p = concat(holes, bay_bosses)) translate([p[0], p[1], z_pcb_top - 1]) cylinder(d=screw_pilot, h=tip_z + 0.3 - z_pcb_top + 1);
        // foro LDR
        if (ldr_hole) translate([ldr_x, ldr_y, z_bez_under - 1]) {
            cylinder(d=ldr_hole_d, h=10);
            translate([0,0,1 + lip_t - 0.6]) cylinder(d1=ldr_hole_d, d2=ldr_hole_d + 1.2, h=0.61);
        }
        usb_cuts();
        if (sd_cutout) translate([sd_x0 - 1, out_y0 - 1, sd_cut_z0]) cube([sd_x1 - sd_x0 + 2, wall + 2, sd_cut_z1 - sd_cut_z0]);
    }
}

/* ================= Diffusore LED ================= */
module diffuser() { cylinder(d=led_diff_d - 2*clr, h=led_diff_t); }

/* ================= Supporto da tavolo ================= */
sa = stand_angle;
ledge_t = 3;
P0 = [0, stand_base_t + ledge_t*cos(sa) + stand_thick*sin(sa)];
function tw(s, t) = P0 + s*[cos(sa), -sin(sa)] + t*[sin(sa), cos(sa)];   // (s,t) inclinati -> profilo

module stand_profile(recess=0) {
    D = box_T + 0.6;
    base_x0 = tw(-(D + stand_lip_t), 0)[0] - 3;
    base_x1 = tw(stand_thick, stand_back)[0] + 18;
    union() {
        translate([base_x0, 0]) square([base_x1 - base_x0, stand_base_t]);
        // schienale
        polygon([tw(0, -ledge_t), tw(stand_thick, -ledge_t), tw(stand_thick, stand_back), tw(0, stand_back)]);
        // mensola + bordo frontale, riempita fino alla base
        hull() {
            polygon([tw(-D - stand_lip_t, -ledge_t), tw(stand_thick, -ledge_t), tw(stand_thick, -recess), tw(-D - stand_lip_t, -recess)]);
            translate([tw(-D - stand_lip_t, -ledge_t)[0], 0]) square([tw(stand_thick, -ledge_t)[0] - tw(-D - stand_lip_t, -ledge_t)[0], 1]);
        }
        polygon([tw(-D - stand_lip_t, -ledge_t), tw(-D, -ledge_t), tw(-D, stand_lip_h), tw(-D - stand_lip_t, stand_lip_h)]);
        // puntone posteriore
        hull() {
            polygon([tw(stand_thick - 0.1, stand_back*0.75 - 3), tw(stand_thick, stand_back*0.75 - 3),
                     tw(stand_thick, stand_back*0.75 + 3), tw(stand_thick - 0.1, stand_back*0.75 + 3)]);
            translate([base_x1 - 12, 0]) square([10, stand_base_t]);
        }
    }
}

// trasforma coordinate del contenitore -> coordinate del supporto
module on_stand() {
    translate([0, P0[0], P0[1]]) rotate([90 - sa, 0, 0]) translate([-xc, -out_y0, -z_bot]) children();
}

module stand() {
    inner = stand_w/2 - stand_pad_w;
    rotate([90, 0, 90]) {
        for (sx = [-1, 1]) translate([0, 0, sx > 0 ? inner : -stand_w/2]) linear_extrude(stand_pad_w) stand_profile(0);
        translate([0, 0, -inner]) linear_extrude(2*inner) stand_profile(1.5);
    }
    if (stand_nubs) on_stand() for (k = keys)
        if (abs(k[0] - xc) < stand_w/2 - 4)
            translate([k[0], k[1], z_bot - stand_thick/2]) cylinder(d1=key_big - 0.6, d2=key_big - 1.4, h=stand_thick/2 + floor_t - 0.3);
}

/* ================= Scheda (solo anteprima) ================= */
module board() {
    color("gold") difference() {
        translate([-pcb_l/2, -pcb_w/2, 0]) cube([pcb_l, pcb_w, pcb_t]);
        for (h = holes) translate([h[0], h[1], -1]) cylinder(d=hole_d, h=5);
    }
    color("white") translate([disp_cx - disp_l/2, -disp_w/2, pcb_t]) cube([disp_l, disp_w, disp_h - 0.05]);
    color("royalblue") translate([aa_cx - aa_l/2, aa_cy - aa_w/2, z_glass - 0.05]) cube([aa_l, aa_w, 0.06]);
    color("dimgray") {
        translate([-36, -9, -2.8]) cube([18.4, 18, 2.8]);                                // ESP32
        translate([pcb_l/2 - 4.9, usb_y - 3.9, -2.5]) cube([5.5, 7.8, 2.5]);            // micro USB
        translate([sd_x0, -24.9, -1.6]) cube([sd_x1 - sd_x0, 15, 1.6]);                  // microSD
        translate([37.5, 4, -3.4]) cube([4.5, 10, 3.4]);                                 // P1
        for (x = [-32.8, -16.8]) translate([x, -24.9, -3.4]) cube([10, 4.3, 3.4]);       // P3 / CN1
        translate([11.4, 20.6, -4.3]) cube([7.6, 3.2, 4.3]);                             // speaker
        for (b = [btn_rst, btn_boot]) translate([b[0] - 1.75, b[1] - 3, btn_tip_z]) cube([3.5, 6, -btn_tip_z]);
        translate([ldr_x - ldr_sx/2, ldr_y - ldr_sy/2, pcb_t]) cube([ldr_sx, ldr_sy, 0.8]); // LDR
    }
    color("lime") translate([led_x - 2.5, led_y - 2.5, -led_h]) cube([5, 5, led_h]);    // LED RGB
}

/* ================= Selezione ================= */
if (part == "front")     rotate([180, 0, 0]) translate([0, 0, -z_top]) front_bezel();   // faccia sul piatto
else if (part == "back") translate([0, 0, -z_bot]) back_shell();
else if (part == "stand") stand();
else if (part == "diffuser") diffuser();
else if (part == "board") board();
else if (part == "assembly") {
    color("dimgray", 0.95) back_shell();
    board();
    color("darkslategray", 0.9) front_bezel();
}
else if (part == "exploded") {
    color("dimgray") back_shell();
    color("white") translate([led_x, led_y, z_floor_in - led_diff_t - 0.1 + 6]) diffuser();
    translate([0, 0, 14]) board();
    color("darkslategray") translate([0, 0, 34]) front_bezel();
}
else if (part == "onstand") {
    color("peru") stand();
    on_stand() { color("dimgray") back_shell(); board(); color("darkslategray") front_bezel(); }
}
