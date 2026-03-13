#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
    bool isFrozen;
};

struct ProblemStatus {
    bool solved = false;
    int firstSolveTime = 0;
    int wrongAttemptsBefore = 0;  // Wrong attempts before solving or before freeze
    vector<Submission*> frozenSubmissions;  // Submissions made during freeze period
};

struct Team {
    string name;
    map<char, ProblemStatus> problems;
    vector<Submission> submissions;
    int solvedCount = 0;
    int penaltyTime = 0;
    vector<int> solveTimes;
    int rank = 0;
};

class ICPCSystem {
private:
    map<string, Team> teams;
    vector<string> teamOrder;
    bool started = false;
    int durationTime = 0;
    int problemCount = 0;
    bool frozen = false;
    bool hasFlushed = false;

    void calculateRankings() {
        vector<string> names = teamOrder;

        sort(names.begin(), names.end(), [this](const string& a, const string& b) {
            const Team& t1 = teams[a];
            const Team& t2 = teams[b];

            if (t1.solvedCount != t2.solvedCount) {
                return t1.solvedCount > t2.solvedCount;
            }
            if (t1.penaltyTime != t2.penaltyTime) {
                return t1.penaltyTime < t2.penaltyTime;
            }

            for (int i = (int)t1.solveTimes.size() - 1, j = (int)t2.solveTimes.size() - 1;
                 i >= 0 && j >= 0; i--, j--) {
                if (t1.solveTimes[i] != t2.solveTimes[j]) {
                    return t1.solveTimes[i] < t2.solveTimes[j];
                }
            }

            return a < b;
        });

        for (size_t i = 0; i < names.size(); i++) {
            teams[names[i]].rank = i + 1;
        }
    }

    string getProblemStatusStr(const Team& team, char prob) {
        auto it = team.problems.find(prob);
        if (it == team.problems.end()) {
            return ".";
        }

        const ProblemStatus& ps = it->second;

        if (ps.solved) {
            if (ps.wrongAttemptsBefore == 0) {
                return "+";
            } else {
                return "+" + to_string(ps.wrongAttemptsBefore);
            }
        } else if (!ps.frozenSubmissions.empty()) {
            if (ps.wrongAttemptsBefore == 0) {
                return "0/" + to_string((int)ps.frozenSubmissions.size());
            } else {
                return "-" + to_string(ps.wrongAttemptsBefore) + "/" + to_string((int)ps.frozenSubmissions.size());
            }
        } else {
            if (ps.wrongAttemptsBefore == 0) {
                return ".";
            } else {
                return "-" + to_string(ps.wrongAttemptsBefore);
            }
        }
    }

    void printScoreboard() {
        for (const string& name : teamOrder) {
            const Team& team = teams[name];
            cout << team.name << " " << team.rank << " " << team.solvedCount
                 << " " << team.penaltyTime;

            for (int i = 0; i < problemCount; i++) {
                char prob = 'A' + i;
                cout << " " << getProblemStatusStr(team, prob);
            }
            cout << "\n";
        }
    }

public:
    void addTeam(const string& name) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
        } else if (teams.find(name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
        } else {
            Team t;
            t.name = name;
            teams[name] = t;
            teamOrder.push_back(name);
            cout << "[Info]Add successfully.\n";
        }
    }

    void startCompetition(int duration, int problems) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
        } else {
            started = true;
            durationTime = duration;
            problemCount = problems;
            cout << "[Info]Competition starts.\n";
        }
    }

    void submit(const string& problem, const string& teamName,
                const string& status, int time) {
        Team& team = teams[teamName];
        char prob = problem[0];

        Submission sub;
        sub.problem = problem;
        sub.status = status;
        sub.time = time;
        sub.isFrozen = false;

        team.submissions.push_back(sub);
        Submission* subPtr = &team.submissions.back();

        ProblemStatus& ps = team.problems[prob];
        bool wasSolvedBefore = ps.solved;

        if (!frozen || wasSolvedBefore) {
            // Not frozen or problem was already solved
            if (status == "Accepted" && !ps.solved) {
                ps.solved = true;
                ps.firstSolveTime = time;
                team.solvedCount++;
                team.penaltyTime += 20 * ps.wrongAttemptsBefore + time;
                team.solveTimes.push_back(time);
                sort(team.solveTimes.begin(), team.solveTimes.end());
            } else if (status != "Accepted" && !ps.solved) {
                ps.wrongAttemptsBefore++;
            }
        } else {
            // Frozen and problem not solved
            subPtr->isFrozen = true;
            ps.frozenSubmissions.push_back(subPtr);
        }
    }

    void flush() {
        calculateRankings();
        hasFlushed = true;
        cout << "[Info]Flush scoreboard.\n";
    }

    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
        } else {
            frozen = true;
            cout << "[Info]Freeze scoreboard.\n";
        }
    }

    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // First flush the scoreboard
        calculateRankings();
        printScoreboard();

        // Continue until no teams have frozen problems
        while (true) {
            // Find the lowest-ranked team with frozen problems
            string lowestTeam = "";
            int lowestRank = -1;

            for (const string& name : teamOrder) {
                Team& team = teams[name];
                bool hasFrozen = false;

                for (int i = 0; i < problemCount; i++) {
                    char prob = 'A' + i;
                    if (!team.problems[prob].frozenSubmissions.empty()) {
                        hasFrozen = true;
                        break;
                    }
                }

                if (hasFrozen && team.rank > lowestRank) {
                    lowestRank = team.rank;
                    lowestTeam = name;
                }
            }

            if (lowestTeam.empty()) break;

            Team& team = teams[lowestTeam];

            // Find the smallest problem number with frozen status
            char minFrozenProb = 0;
            for (int i = 0; i < problemCount; i++) {
                char prob = 'A' + i;
                if (!team.problems[prob].frozenSubmissions.empty()) {
                    minFrozenProb = prob;
                    break;
                }
            }

            ProblemStatus& ps = team.problems[minFrozenProb];
            int oldRank = team.rank;

            // Process frozen submissions for this problem
            for (Submission* sub : ps.frozenSubmissions) {
                if (sub->status == "Accepted" && !ps.solved) {
                    ps.solved = true;
                    ps.firstSolveTime = sub->time;
                    team.solvedCount++;
                    team.penaltyTime += 20 * ps.wrongAttemptsBefore + sub->time;
                    team.solveTimes.push_back(sub->time);
                    sort(team.solveTimes.begin(), team.solveTimes.end());
                } else if (sub->status != "Accepted" && !ps.solved) {
                    ps.wrongAttemptsBefore++;
                }
            }

            ps.frozenSubmissions.clear();

            // Recalculate rankings
            calculateRankings();

            // Check if ranking changed
            if (team.rank < oldRank) {
                string replaced = "";
                for (const string& n : teamOrder) {
                    if (teams[n].rank == oldRank && n != lowestTeam) {
                        replaced = n;
                        break;
                    }
                }
                cout << team.name << " " << replaced << " "
                     << team.solvedCount << " " << team.penaltyTime << "\n";
            }
        }

        printScoreboard();
        frozen = false;
    }

    void queryRanking(const string& name) {
        if (teams.find(name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
        } else {
            cout << "[Info]Complete query ranking.\n";
            if (frozen) {
                cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
            }
            if (!hasFlushed) {
                // Before first flush, rank by lexicographic order
                vector<string> sorted = teamOrder;
                sort(sorted.begin(), sorted.end());
                int rank = 1;
                for (const string& n : sorted) {
                    if (n == name) break;
                    rank++;
                }
                cout << name << " NOW AT RANKING " << rank << "\n";
            } else {
                cout << name << " NOW AT RANKING " << teams[name].rank << "\n";
            }
        }
    }

    void querySubmission(const string& name, const string& problem,
                         const string& status) {
        if (teams.find(name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
        } else {
            cout << "[Info]Complete query submission.\n";

            const Team& team = teams[name];
            Submission found;
            bool foundAny = false;

            for (int i = team.submissions.size() - 1; i >= 0; i--) {
                const Submission& sub = team.submissions[i];
                bool problemMatch = (problem == "ALL" || sub.problem == problem);
                bool statusMatch = (status == "ALL" || sub.status == status);

                if (problemMatch && statusMatch) {
                    found = sub;
                    foundAny = true;
                    break;
                }
            }

            if (foundAny) {
                cout << name << " " << found.problem << " "
                     << found.status << " " << found.time << "\n";
            } else {
                cout << "Cannot find any submission.\n";
            }
        }
    }

    void end() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ICPCSystem system;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "ADDTEAM") {
            string name;
            iss >> name;
            system.addTeam(name);
        } else if (cmd == "START") {
            string duration, problem;
            int dur, prob;
            iss >> duration >> dur >> problem >> prob;
            system.startCompetition(dur, prob);
        } else if (cmd == "SUBMIT") {
            string problem, by, teamName, with, status, at;
            int time;
            iss >> problem >> by >> teamName >> with >> status >> at >> time;
            system.submit(problem, teamName, status, time);
        } else if (cmd == "FLUSH") {
            system.flush();
        } else if (cmd == "FREEZE") {
            system.freeze();
        } else if (cmd == "SCROLL") {
            system.scroll();
        } else if (cmd == "QUERY_RANKING") {
            string name;
            iss >> name;
            system.queryRanking(name);
        } else if (cmd == "QUERY_SUBMISSION") {
            string teamName, where, rest;
            iss >> teamName >> where;
            getline(iss, rest);

            size_t probPos = rest.find("PROBLEM=");
            size_t statusPos = rest.find("STATUS=");

            string problem, status;
            if (probPos != string::npos) {
                size_t start = probPos + 8;
                size_t end = rest.find(" AND", start);
                problem = rest.substr(start, end - start);
            }
            if (statusPos != string::npos) {
                size_t start = statusPos + 7;
                status = rest.substr(start);
            }

            system.querySubmission(teamName, problem, status);
        } else if (cmd == "END") {
            system.end();
            break;
        }
    }

    return 0;
}
