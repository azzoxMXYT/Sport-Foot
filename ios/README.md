# Sport Foot sur iPhone

Version native iOS 15+ : UIKit, Core Graphics et le même moteur C que les versions Windows et Android. Pas de Safari, de WebView ni de connexion réseau pendant le jeu. Tenir l’iPhone en paysage : toucher **Coup d’envoi**, glisser à gauche pour diriger le joueur et appuyer sur **PASSE** ou **TIR** avec l’autre main.

**Avec seulement un iPhone** : cette version n’est pas encore distribuée sur TestFlight ou l’App Store. Le projet Xcode ci-joint n’est pas un installateur. Un développeur doit d’abord publier une version signée pour Apple ; la signature Windows/Android ne peut pas être réutilisée.

### Essayer depuis un Mac

1. Ouvrir `ios/SportFoot.xcodeproj` dans Xcode 16 ou ultérieur. Le projet embarque la couverture et l’icône, sans dépendance à installer.
2. Choisir le schéma **SportFoot** et un simulateur iPhone, puis **Run** (⌘R). Aucun compte Apple n’est requis pour le simulateur.
3. Pour un iPhone physique : le connecter au Mac, sélectionner l’iPhone comme destination et choisir son équipe dans **Signing & Capabilities**. Si nécessaire, remplacer `fr.sportfoot.arcade` par un identifiant d’app unique pour l’équipe. Accepter les demandes de jumelage et, si iOS le demande, activer le mode développeur sur l’iPhone personnel. Puis **Run**. Une équipe personnelle gratuite permet des essais locaux limités dans le temps, mais pas la distribution TestFlight.

### Distribuer via TestFlight

Le responsable de la distribution doit disposer d’une équipe inscrite à l’Apple Developer Program et des droits App Store Connect :

1. Configurer l’équipe et l’identifiant d’app dans Xcode, puis créer la fiche correspondante dans App Store Connect.
2. Avec une destination **Any iOS Device**, utiliser **Product → Archive**, puis **Distribute App → App Store Connect**. Incrémenter le numéro de build lors des envois suivants.
3. Dans TestFlight, attendre le traitement et compléter les informations requises. Pour des testeurs externes, soumettre à la validation bêta si demandée. Une fois la version disponible, partager l’invitation. Le joueur installe alors TestFlight puis accepte l’invitation sur son iPhone.

Ne pas déposer de certificat privé, de profil ni de clé App Store Connect dans ce dépôt. Le workflow `ios.yml` est uniquement un contrôle de compilation et de lancement sur simulateur ; il ne signe pas de distribution et ne publie rien chez Apple.
