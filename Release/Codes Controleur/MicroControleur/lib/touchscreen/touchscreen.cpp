#include "touchscreen.h"  // Gestion de l'écran tactile (XPT2046)
#include "actions.h"      // Accès aux objets LVGL et fonctions utilitaires
#include "BLE.h"          // Partage de l'état via BLE (ex. : stopFlag)

// Définition de la broche connectée au contrôle de rétroéclairage (MOSFET)
#define BACKLIGHT_PIN 17  

// Valeurs PWM pour la gestion de la luminosité
#define FULL_BRIGHTNESS 0     // Luminosité maximale (écran allumé à fond)
#define DIM_BRIGHTNESS 220    // Luminosité réduite (pour mode veille ou inactivité)

// Facteur de conversion pour les délais de veille profonde (microsecondes en secondes)
#define uS_TO_S_FACTOR 1000000ULL

// Variables globales pour la gestion du toucher et de la luminosité
unsigned long lastTouchTime = 0; // Moment du dernier contact avec l'écran
bool isDimmed = false;           // Indique si l'écran est actuellement atténué

/**
 * @brief Fonction de lecture tactile appelée par LVGL.
 * Gère également la remise en pleine luminosité de l’écran en cas d’activité.
 */
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) 
{
  if (touchscreen.touched())  // Un contact tactile est détecté
  {
    TS_Point p = touchscreen.getPoint(); // Lecture des coordonnées brutes

    // Conversion des coordonnées brutes en positions sur l'écran
    x = map(p.x, 200, 3950, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 4050, 1, SCREEN_HEIGHT);
    z = p.z;  // Pression (non utilisée ici)

    // Contrainte pour éviter les coordonnées négatives
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    // Mise à jour des données d'entrée LVGL
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;

    // Mise à jour du dernier moment d'activité
    lastTouchTime = millis();

    // Si l'écran était atténué, on le remet à pleine luminosité
    if (isDimmed) {
      analogWrite(BACKLIGHT_PIN, FULL_BRIGHTNESS);
      isDimmed = false;
    }
  }
  else  // Aucun contact détecté
  {
    data->state = LV_INDEV_STATE_RELEASED;

    // Si le délai d'inactivité est dépassé et que l’écran n’est pas déjà atténué
    if (!isDimmed && millis() - lastTouchTime >= veille_delay) {
        // Ici on déclenche potentiellement une mise en veille ou une transition d’état
        stopFlag = false;  // Flag de contrôle partagé (ex: utilisé par d'autres tâches)
    }
  }
}

/**
 * @brief Action appelée lors d’un clic sur un bouton de sauvegarde.
 * Sauvegarde la configuration et applique une rétroaction visuelle.
 */
void action_save(lv_event_t *e) {
    // Changement de couleur d’arrière-plan d’un objet (ex. : confirmation visuelle)
    lv_obj_set_style_bg_color(objects.obj9, lv_color_hex(0xff2afc), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Sauvegarde des paramètres dans la mémoire persistante
    putConfig();

    // Affiche un texte en rendant le texte complètement opaque
    lv_obj_set_style_text_opa(objects.obj8, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/**
 * @brief Action appelée lors d’un clic sur un bouton « Quitter ».
 * Peut être utilisée pour quitter un écran, une session, ou éteindre l’interface.
 */
void action_quit(lv_event_t *e) {
    // Ancien appel à putConfig() commenté (non utilisé ici)

    // Signal que l’application devrait quitter ou suspendre l’activité
    stopFlag = false;
}

/**
 * @brief Action LVGL : changement de couleur d’un objet (ex. : feedback visuel).
 */
void action_change_color(lv_event_t * e){
    // Applique une nouvelle couleur d’arrière-plan à l’objet visé
    lv_obj_set_style_bg_color(objects.obj9, lv_color_hex(0xff1fbd00), LV_PART_MAIN | LV_STATE_DEFAULT);
}
