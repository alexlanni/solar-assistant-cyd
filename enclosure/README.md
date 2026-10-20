# Contenitore CYD Solar Monitor (ESP32-2432S028)

Contenitore stampabile in 3D per il "Cheap Yellow Display" (display 2.8" ILI9341 320x240, touch resistivo) montato **in orizzontale**: **USB a destra, slot microSD in basso, fotoresistenza (LDR) a sinistra del display**. È l'orientamento di `tft.setRotation(1)`, quello usato dal firmware. Si può appendere a parete oppure appoggiare sul supporto da tavolo.

Il sorgente è interamente parametrico: `cyd_enclosure.scad` (OpenSCAD 2021.01 o successivo).

## File

| File | Cosa contiene |
|---|---|
| `cyd_enclosure.scad` | sorgente parametrico (tutte le quote sono variabili in testa al file) |
| `cyd_front.stl` | cornice frontale, già orientata per la stampa (faccia a vista sul piatto) |
| `cyd_back.stl` | guscio posteriore, già orientato per la stampa (fondo sul piatto) |
| `cyd_stand.stl` | supporto da tavolo inclinato di 18° (base sul piatto) |
| `cyd_diffuser.stl` | dischetto diffusore per il LED RGB (Ø 8,6 x 0,7 mm) |
| `preview_*.png` | anteprime: assemblato, esploso, singole parti, sul supporto |

Ingombro esterno: **120,8 x 54,8 x 15,5 mm**. Tutte le parti stanno su un piatto da 220 x 220 mm.

Per rigenerare i file:
```
openscad -D 'part="front"' -o cyd_front.stl cyd_enclosure.scad
openscad -D 'part="back"'  -o cyd_back.stl  cyd_enclosure.scad
openscad -D 'part="stand"' -o cyd_stand.stl cyd_enclosure.scad
openscad -D 'part="diffuser"' -o cyd_diffuser.stl cyd_enclosure.scad
```
Le viste di anteprima si ottengono con `part="assembly"`, `"exploded"` oppure `"onstand"`.

## Com'è fatto

- **Due gusci con battuta a mezzo spessore.** Il PCB è stretto tra le 4 torrette del guscio posteriore e i 4 perni della cornice. Le **stesse 4 viti M3**, avvitate dal retro, passano nei fori del PCB e fanno presa nei perni della cornice. Altre 2 viti chiudono il vano cavo. Davanti non si vede nessuna vite.
- **Finestra del display** = area attiva + 1 mm per lato, con smusso a 45°. Il bordo sopra il vetro è spesso 1,2 mm e resta a 0,4 mm dal vetro, così la cornice non preme sul touch resistivo (niente tocchi fantasma) e si riesce a toccare fino al bordo.
- **Foro per la LDR** in basso a sinistra, con un pozzetto che scherma la luce della retroilluminazione: senza lo schermo la LDR leggerebbe il display stesso.
- **Vano cavo USB a destra (30 mm).** La spina micro-USB resta dentro il contenitore e il cavo curva verso il basso. Di serie esce da un **passacavo a U nel fianco inferiore**, che stringe leggermente il cavo, e si blocca con una **fascetta** che passa nelle due asole del fondo (antistrappo). Con `cable_exit="back"` esce invece da un foro nel retro, per il cavo che arriva dal muro. Con `bay_len=0` il vano sparisce e la spina esce dal fianco destro. In questo caso `usb_dual=true` apre anche il foro per la USB-C della variante a doppia USB.
- **RST e BOOT**: due fori da 1,8 mm sul retro per una graffetta, marcati "R" e "B", con guide interne fino a 0,6 mm dal pulsante.
- **Ventilazione**: feritoie sul retro (per l'uso da tavolo) e sui fianchi superiore e inferiore (convezione quando è appeso al muro).
- **Fessura microSD** nel fianco inferiore.
- **Finestra per il LED RGB sul retro**, per il "respiro" verde/rosso del firmware:
  - foro Ø 7 mm sul retro;
  - sede interna Ø 9 mm per il diffusore stampato (`cyd_diffuser.stl`);
  - **tubetto anti-dispersione** dal fondo fino a 1 mm dal PCB, così la luce non si sparge dentro il contenitore;
  - **canalino sul retro** (4 x 1 mm) dal LED al bordo inferiore: appeso al muro, il LED crea un alone che esce dal bordo in basso.

  In alternativa, `led_window_mode="membrane"` lascia una membrana di 0,6 mm integrata nel fondo. Ha senso solo se il guscio è stampato in un colore chiaro o naturale.
- **Montaggio a parete**: due asole a buco di serratura (interasse 56 mm). La testa della vite entra nel foro grande, poi si fa scendere il contenitore. All'interno, un recinto attorno all'asola guida la testa e la separa dall'elettronica.
- **Supporto da tavolo**: il contenitore si appoggia sulla mensola inclinata di 18°. Due pioli del supporto entrano nei fori grandi delle asole e lo bloccano sui lati. Basta appoggiarlo, e lo si toglie sollevandolo.

## Stampa

| Parte | Orientamento | Supporti | Note |
|---|---|---|---|
| `cyd_front.stl` | faccia a vista sul piatto (già così nell'STL) | **no** | lo smusso della finestra è a 45° |
| `cyd_back.stl` | fondo sul piatto (già così nell'STL) | **no** | le lettere R/B sono incise sul lato del piatto |
| `cyd_stand.stl` | base sul piatto | **no** | schienale inclinato di 18°; pioli piccoli, nessun problema |
| `cyd_diffuser.stl` | piatto | no | PETG/PLA **naturale o trasparente**, 2 layer da 0,35 mm o 3-4 da 0,2 mm, riempimento 100% |

- **Materiale**: **PETG** consigliato (regge il calore in casa e vicino a finestre o al sole; il display e l'ESP32 scaldano poco). Il PLA va bene se resta lontano dal sole. Per un buon effetto alone, stampa il guscio posteriore in colore scuro e il diffusore in naturale.
- **Layer**: 0,2 mm (0,16 mm per la cornice se vuoi lo smusso più pulito). Ugello 0,4 mm.
- **Pareti**: 3 perimetri, 4 layer sopra e sotto, riempimento 20-25% (gyroid). Il supporto può andare al 15%.
- La prima stampa conviene farla solo della cornice per verificare la finestra (vedi sotto).

## Minuteria

| Q.tà | Articolo | Uso |
|---|---|---|
| 6 | Vite **M3 x 12** a testa bombata ISO 7380 o cilindrica bassa DIN 7985 (testa Ø ≤ 6 mm, altezza ≤ 2,5 mm) | 4 per PCB + cornice, 2 per il vano cavo. Si autofilettano nei prefori Ø 2,5 della cornice |
| 1 | Fascetta stretta (2,5 mm) | antistrappo del cavo USB |
| 2 | Viti da muro Ø 3,5-4 mm con testa Ø 7-8 mm (testa ≤ 2,6 mm) + tasselli | montaggio a parete |
| – | (opzionale) 4 piedini in gomma adesivi Ø 8 mm | sotto il supporto |

Le viti autofilettanti M3 nel PETG o nel PLA vanno bene per pochi montaggi. Se prevedi di aprirlo spesso, porta `screw_pilot` a 2,6-2,8 e usa viti per plastica (tipo PT/Delta da 3 mm). In alternativa usa la versione M2.5: `screw_hole=2.9`, `screw_pilot=2.1`, `head_d=5.5`.

## Montaggio

1. Prova a secco la cornice sulla scheda: il vetro non deve toccare la cornice e la finestra deve essere centrata sull'immagine.
2. Se usi `insert`, metti il **diffusore** nella sede interna del LED sul fondo del guscio (una goccia di colla o un leggero incastro).
3. Appoggia la scheda (display verso l'alto, USB verso il vano) sulle 4 torrette del guscio. I pulsanti RST/BOOT vanno sopra i due fori in alto a sinistra.
4. Inserisci il cavo micro-USB, fallo curvare nel vano e premilo nel passacavo a U del fianco inferiore. Bloccalo con la fascetta nelle due asole del fondo.
5. Appoggia la cornice: la linguetta chiude il passacavo e la battuta si incastra nel guscio.
6. Gira il tutto e avvita dal retro le 6 viti M3 x 12 senza stringere troppo: la scheda resta stretta tra torrette e perni.
7. **A parete**: due viti a 56 mm di interasse, alla stessa altezza, che sporgano circa 3 mm. Infila le teste nei fori grandi e fai scendere il contenitore di 8 mm. **Sul tavolo**: appoggialo sul supporto con i fori delle asole sui due pioli.

## Quote da verificare col calibro (prima di stampare)

Le quote del PCB vengono dal disegno ufficiale e sono affidabili. Tutto il resto è ricavato da modelli 3D o foto della community e va **misurato sulla propria scheda**: esistono più revisioni del CYD. Le coordinate sono in mm dal **centro del PCB**, con X verso destra e Y verso l'alto guardando lo schermo con l'USB a destra; z = 0 è la faccia posteriore del PCB.

| Parametro | Valore | Affidabilità | Come misurare |
|---|---|---|---|
| `pcb_l`, `pcb_w`, `hole_dx`, `hole_dy`, `hole_d` | 86 x 50, fori 78 x 42, Ø 3,2 | **ufficiale** (disegno Jingcai) | – |
| `aa_l`, `aa_w`, `aa_off_x`, `disp_l` | 57,6 x 43,2; 3,05 dal bordo; 69,2 | **ufficiale** (datasheet LCD JC2432A028N) | – |
| `disp_cx` | 0 (display centrato sul PCB) | stimato (modello STEP) | distanza bordo PCB sinistro ↔ bordo display sinistro deve essere 8,4 |
| `disp_h` | 3,3 (vetro sopra la faccia frontale del PCB) | **incerto** (modello STEP; il datasheet dice 2,3 + biadesivo 0,3 per il solo LCD) | spessore totale scheda+display − 1,6. **Molto importante per il touch.** |
| `pcb_t` | 1,6 | standard | calibro |
| `ldr_x`, `ldr_y` | −39,5 ; −14 | stimato (modello STEP + foto) | centro della LDR sul fronte |
| `led_x`, `led_y` | −12,5 ; −14 | **stimato, da verificare** (modello STEP, foto del produttore) | centro del LED RGB sul retro. Il foro Ø 7 tollera circa ±2 mm |
| `led_pipe_gap` | 1,0 | stimato | se ci sono componenti più alti di 0,8 mm attorno al LED, aumentalo |
| `btn_rst`, `btn_boot`, `btn_tip_z` | (−33,65; 20,2), (−29,65; 20,2), −4,6 | stimato (modello STEP) | centro dei pulsanti sul retro |
| `usb_y`, `usb_zc` | −9,3 ; −1,25 | stimato (modello STEP) | centro della presa micro-USB |
| `usb_dual`, `usbc_y` | false ; +4 | **non verificato** (nessun disegno trovato per la variante a 2 USB) | solo se `bay_len=0` |
| `sd_x0`, `sd_x1` | −2,6 … 12,4 | stimato | bordi dello slot microSD |
| `rear_clear` | 7 | stimato (componente più alto sul retro: pulsanti/JST ≈ 4,6) | altezza massima dei componenti sul retro + 2 |
| `usb_plug_len`, `cable_d` | 16 ; 4 | dipende dal cavo | lunghezza della parte rigida della spina e diametro del cavo |

Altri parametri utili: `clr` (gioco FDM, 0,2), `pcb_clr` (gioco attorno a PCB/display, 0,4), `disp_gap` (aria sopra il vetro), `lip_t` (spessore bordo), `win_margin`, `bay_len`, `cable_exit`, `led_window_mode`, `stand_angle` (15-20°), `key_dx`.

**Promemoria:** il LED RGB (IO4/IO16/IO17) è sul **retro** della scheda. Controlla che la finestra `led_x/led_y` cada sopra il LED della tua scheda prima di stampare il guscio. Puoi stampare solo i primi 2 mm del guscio (taglia l'STL nello slicer) e appoggiarci la scheda.

## Fonti

- Disegno quotato ufficiale del PCB (86 x 50, fori a 4 mm dai bordi, R 1,60) e specifiche "ESP32-2432S028R" di Shenzhen Jingcai: <https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/tree/main/OriginalDocumentation> (`3-Structure_Diagram/Dimensions.png`, `2-Specification/ESP32-2432S028 Specifications-EN.pdf`)
- Disegno meccanico del modulo LCD JC2432A028N (69,2 x 50 x 2,3; AA 57,6 x 43,2; AA a 3,05 / 3,40 dai bordi): `OriginalDocumentation/4-Driver_IC_Data_Sheet/JC2432A028N.pdf` nello stesso repository
- Modello 3D della scheda (posizioni di USB, pulsanti, SD, JST, LED, LDR): Havenview, `3dModels/Havenview_CYD_CAD/Cheap Yellow Display v16.step` nello stesso repository
- Conferma della posizione della LDR sul fronte e della necessità di schermarla dalla retroilluminazione: `3dModels/Markus_CYD_Simple_Case` nello stesso repository
- Specifiche generali: <https://mischianti.org/esp32-2432s028-cheap-yellow-display-high-resolution-pinout-datasheet-schema-and-specs>

## Limiti noti

- Le posizioni dei componenti vengono da un modello 3D della community, non da un disegno ufficiale (vedi tabella).
- Il vano cavo rende il contenitore asimmetrico (circa 30 mm in più a destra). Se usi un cavo micro-USB **a 90°** puoi ridurre `bay_len` a circa 15-18 mm.
- Con il supporto da tavolo il cavo esce dal fianco inferiore, a destra del supporto: il supporto è largo 80 mm e il passacavo resta fuori.
- Le autofilettanti nella plastica non sono pensate per aperture frequenti (vedi Minuteria).
