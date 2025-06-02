#include <iostream>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

struct Tache
{
    int id;
    string titre;
    bool terminee;
};

void ajouterTache(vector<Tache> &taches)
{
    Tache t;
    cout << "Entrez le titre de la tâche : ";
    cin.ignore();
    getline(cin, t.titre);
    t.id = taches.empty() ? 1 : taches.back().id + 1;
    t.terminee = false;
    taches.push_back(t);
    cout << "Tâche ajoutée avec succès." << endl;
}

void afficherTaches(const vector<Tache> &taches)
{
    if (taches.empty())
    {
        cout << "Aucune tâche enregistrée." << endl;
        return;
    }

    for (const auto &t : taches)
    {
        cout << "[" << (t.terminee ? "X" : " ") << "] "
             << t.id << ". " << t.titre << endl;
    }
}

void marquerCommeTerminee(vector<Tache> &taches)
{
    int id;
    cout << "ID de la tâche à marquer comme terminée : ";
    cin >> id;
    for (auto &t : taches)
    {
        if (t.id == id)
        {
            t.terminee = true;
            cout << "Tâche marquée comme terminée." << endl;
            return;
        }
    }
    cout << "Tâche non trouvée." << endl;
}

void supprimerTache(vector<Tache> &taches)
{
    int id;
    cout << "ID de la tâche à supprimer : ";
    cin >> id;
    for (auto it = taches.begin(); it != taches.end(); ++it)
    {
        if (it->id == id)
        {
            taches.erase(it);
            cout << "Tâche supprimée." << endl;
            return;
        }
    }
    cout << "Tâche non trouvée." << endl;
}

void sauvegarder(const vector<Tache> &taches)
{
    ofstream fichier("taches.txt");
    for (const auto &t : taches)
    {
        fichier << t.id << ";" << t.titre << ";" << t.terminee << endl;
    }
}

void charger(vector<Tache> &taches)
{
    ifstream fichier("taches.txt");
    if (!fichier)
        return;

    Tache t;
    string ligne;
    while (getline(fichier, ligne))
    {
        size_t p1 = ligne.find(';');
        size_t p2 = ligne.rfind(';');
        if (p1 == string::npos || p2 == string::npos)
            continue;
        t.id = stoi(ligne.substr(0, p1));
        t.titre = ligne.substr(p1 + 1, p2 - p1 - 1);
        t.terminee = ligne.substr(p2 + 1) == "1";
        taches.push_back(t);
    }
}

int main()
{
    vector<Tache> taches;
    charger(taches);
    int choix;
    do
    {
        cout << "\n--- Gestionnaire de tâches ---\n";
        cout << "1. Ajouter une tâche\n";
        cout << "2. Afficher les tâches\n";
        cout << "3. Marquer comme terminée\n";
        cout << "4. Supprimer une tâche\n";
        cout << "5. Quitter\n";
        cout << "Choix : ";
        cin >> choix;

        switch (choix)
        {
        case 1:
            ajouterTache(taches);
            break;
        case 2:
            afficherTaches(taches);
            break;
        case 3:
            marquerCommeTerminee(taches);
            break;
        case 4:
            supprimerTache(taches);
            break;
        case 5:
            sauvegarder(taches);
            cout << "Au revoir !" << endl;
            break;
        default:
            cout << "Choix invalide." << endl;
        }
    } while (choix != 5);

    return 0;
}
