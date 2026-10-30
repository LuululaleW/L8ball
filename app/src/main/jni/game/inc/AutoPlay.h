#pragma once

#include "Prediction.fast.h"
#include <imgui/imgui.h>
#include <algorithm>

#include "ScreenTable.h"

// #include "PowerSlider.h"
#include "mod/ButtonClicker.h"

using namespace ImGui;

constexpr double maxAngle = 360.0 / (180.0 / M_PI);

double normalizeAngle(double angle) {
    double newAngle = angle;
    if (newAngle >= maxAngle) newAngle = fmod(newAngle, maxAngle);
    else if (newAngle < 0) newAngle = maxAngle - fmod(-newAngle, maxAngle);
    return newAngle;
}

Candidate g_CurrentCandidate = { -1 };

ImVec2 GetPocketScreenPos(int pocketIdx) {
    Table table = sharedGameManager.mTable;
    if (!table) return {};

    auto tableProperties = table.mTableProperties();
    if (!tableProperties) return {};

    auto& pockets = tableProperties.mPockets();
    return WorldToScreen(pockets[pocketIdx]);
}

bool IsShotValid() {
    auto& cand = g_CurrentCandidate;
    if (cand.idx == -1) return false;

    Ball::Classification myclass = sharedGameManager.getPlayerClassification();

    uint nominatedPocket = sharedGameManager.getNominatedPocket();
    if (nominatedPocket < 6 && cand.pocketIndex != nominatedPocket) return false;

    if (!gPrediction->guiData.balls[0].onTable) return false; // cue ball should not be pocketed
    if (!gPrediction->guiData.balls[cand.idx].originalOnTable) return false; // target ball was already potted
    if (gPrediction->guiData.balls[cand.idx].onTable) return false; // target ball was not potted
    if (gPrediction->guiData.balls[cand.idx].pocketIndex != cand.pocketIndex) return false; // target ball did not go into target pocket

    auto& ball8 = gPrediction->guiData.balls[8];
    if (myclass == Ball::Classification::ANY && ball8.originalOnTable && !ball8.onTable) return false;

    auto& firstHit = gPrediction->guiData.collision.firstHitBall;
    if (firstHit) {
        if (myclass == Ball::Classification::ANY) {
            if (firstHit->classification == Ball::Classification::EIGHT_BALL) return false;
        } else if (firstHit->classification != myclass) return false;
    }

    return true;
}

/* void UpdatePowerSlider() {
    static bool isSearchingExtraPower = false;
    static float savedLowestPower = 0.0f;
    static ImVec2 savedLowestPos = ImVec2(0, 0);
    static ImVec2 bestValidPos = ImVec2(0, 0);

    if (powerSlider.state != powerSlider.State::MOVING) {
        isSearchingExtraPower = false;
        return;
    }

    float currentPower = sharedGameManager.mVisualCue().getShotPower(true);
    bool isValid = IsShotValid();

    if (!isSearchingExtraPower) {
        if (isValid && currentPower < 666.0f) {
            isSearchingExtraPower = true;
            savedLowestPower = currentPower;
            savedLowestPos = powerSlider.CurrentPos;
            bestValidPos = powerSlider.CurrentPos;
            LOGI("Found valid shot at %f, searching for more power...", currentPower);
        }
    } else {
        if (isValid) bestValidPos = powerSlider.CurrentPos;

        if (!isValid) {
            float midX = bestValidPos.x;// + (powerSlider.CurrentPos.x - savedLowestPos.x) * 0.5f;
            float midY = bestValidPos.y;// + (powerSlider.CurrentPos.y - savedLowestPos.y) * 0.5f;
            
            powerSlider.CurrentPos = ImVec2(midX, midY);
            NativeTouchesMove(powerSlider.TouchIndex, midX, midY);
            
            LOGI("Shot invalid at %f. Backing off and ending.", currentPower);
            powerSlider.state = powerSlider.State::ENDING;
            isSearchingExtraPower = false;
        } else if (currentPower >= savedLowestPower + 80.0f) {
            LOGI("Reached +80 power (%f -> %f). Forcefully ending.", savedLowestPower, currentPower);
            powerSlider.state = powerSlider.State::ENDING;
            isSearchingExtraPower = false;
        }
    }
} */

Point2D lastFailedCuePos = { -1000.0, -1000.0 };

// Declared in draw.h
extern float g_toggleRotAngle;       // kept for linker compatibility
extern bool  g_autoPlayCalculating;  // true while SLOW scan is running

namespace AutoPlay {
    double lastSetAngle = 0.f;
    bool didSetAngle = false;
    bool bAutoPlaying = false;

    enum State {
        IDLE,           // Waiting for player turn or Autoplay to be enabled
        SCANNING,       // Searching for the best shot candidate (calculating physics)
        NOMINATING,     // Waiting for pocket nomination click to finish
        EXECUTING,      // Setting the angle and spin (waiting for visual cue to update)
    } state = IDLE;
    
    double pendingShotPower = 0.f;
    double pendingShotAngle = 0.f;
    int nominationFrameCounter = 0;
    
    enum ScanMode {
        FAST,
        SLOW,
    } scan = FAST;

    bool shouldAutoPlay() { return !didSetAngle || lastSetAngle == sharedGameManager.mVisualCue().mVisualGuide().mAimAngle(); }

    void setAimAngle(double angle) {
        lastSetAngle = angle;
        sharedGameManager.mVisualCue().mVisualGuide().mAimAngle(angle);
    }

    void takeShot(double angle, double power) {
        setAimAngle(angle);
        gPrediction->determineShotResult(false, angle, power);

        sharedGameManager.mVisualCue().mPower(ShotPowerToPower(power));
        M(void, libmain + 0x2dc0c58, void*)(F(void*, sharedGameManager + 0x3b0));
    }
    
    void ClearState() {
        g_CurrentCandidate.idx = -1;
        lastFailedCuePos = { -1000.0, -1000.0 };
    }
    
    void Shoot(double angle, double power = 0.f) {
        setAimAngle(angle);
        gPrediction->determineShotResult(false, angle, power);

        bool nominating = false;
        int nominationMode = sharedGameManager.getPocketNominationMode();
        auto myclass = sharedGameManager.getPlayerClassification();
        if ((nominationMode == 1 && myclass == Ball::Classification::EIGHT_BALL) || (nominationMode == 2 && myclass != Ball::Classification::ANY)) {
            if (g_CurrentCandidate.idx != -1 && sharedGameManager.getNominatedPocket() != g_CurrentCandidate.pocketIndex) {
                nominating = true;
            }
        }

        if (nominating) {
            pendingShotPower = power;
            pendingShotAngle = angle;
            state = NOMINATING;
            nominationFrameCounter = 0;
        } else {
            takeShot(angle, power);
            ClearState();
            state = IDLE;
        }
    }
    
    void ScanSlow(double angleStep = 0.01f) {
        static double currentScanAngle = 0.0;
        static bool isScanning = false;
        static Point2D lastScanCuePos = { -1000.0, -1000.0 };

        if (g_CurrentCandidate.idx != -1) return;
        
        // Reset if we just started or wrapped around, or if table changed
        if (!isScanning || gPrediction->guiData.balls[0].initialPosition != lastScanCuePos) {
            currentScanAngle = 0.0;
            isScanning = true;
            lastScanCuePos = gPrediction->guiData.balls[0].initialPosition;
        }

        Ball::Classification myclass = sharedGameManager.getPlayerClassification();
        uint nominatedPocket = sharedGameManager.getNominatedPocket();
        auto& cueBall = gPrediction->guiData.balls[0];
        
        int steps = 0;
        bool foundShot = false;
        
        // Scan 10 angles per frame
        while (steps < 10 && currentScanAngle < maxAngle) {
            double angle = currentScanAngle;
            currentScanAngle += angleStep;
            steps++;

            std::vector<double> powers = {666.0, 466.0, 266.0, 100.0};
            for (double power : powers) {
                gPrediction->determineShotResult(true, angle, power, sharedGameManager.getShotSpin());
                
                bool isPotentiallyValid = false;
                int targetIdx = -1;

                bool bFoundLowestNumberedBall = false;
                int iFoundLowestNumberedBall = -1;
                bool isNineBallGame = myclass == Ball::Classification::NINE_BALL_RULE;

                if (isNineBallGame) {
                    for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                        auto& ball = gPrediction->guiData.balls[i];
                        if (!ball.originalOnTable) continue; // skip already potted

                        bFoundLowestNumberedBall = true;
                        iFoundLowestNumberedBall = i;
                        break;
                    }

                    auto firstHit = gPrediction->guiData.collision.firstHitBall;
                    if (!firstHit) continue;
                    
                    // Must hit lowest numbered ball first
                    if (firstHit->index != iFoundLowestNumberedBall) continue;

                    // Cue ball must stay on table
                    if (!gPrediction->guiData.balls[0].onTable) continue;

                    int bestPottedIdx = -1;
                    for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                        auto& ball = gPrediction->guiData.balls[i];
                        if (ball.originalOnTable && !ball.onTable) {
                            if (nominatedPocket < 6 && ball.pocketIndex != nominatedPocket) continue;
                            
                            if (i == 9) { bestPottedIdx = 9; break; }
                            if (bestPottedIdx == -1 || i == firstHit->index) bestPottedIdx = i;
                        }
                    }

                    if (bestPottedIdx == -1) continue;
                    targetIdx = bestPottedIdx;

                    LOGI("AutoPlay: 9ball: Found good angle %f with power %f", angle, power);
                    
                    g_CurrentCandidate.idx = targetIdx;
                    g_CurrentCandidate.angle = angle;
                    g_CurrentCandidate.power = power;
                    g_CurrentCandidate.pocketIndex = gPrediction->guiData.balls[targetIdx].pocketIndex;

                    foundShot = true;
                    Shoot(angle, power);
                    break;
                }

                // Check if ANY valid ball is potted
                for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                    auto& ball = gPrediction->guiData.balls[i];
                    if (ball.originalOnTable && !ball.onTable) { // Ball was potted
                        bool isValidTarget = false;
                        // Logic for valid target (Simplified)
                        // If table is open (ANY), any ball except 8-ball and Cue-ball is valid.
                        // If class is assigned, only that class is valid.
                        // Note: If on 8-ball, usually class logic might differ, but assuming standard flow:
                        
                        if (myclass == Ball::Classification::ANY) {
                            if (ball.classification != Ball::Classification::CUE_BALL && ball.classification != Ball::Classification::EIGHT_BALL) isValidTarget = true;
                        } else {
                            if (ball.classification == myclass) isValidTarget = true;
                        }
                        
                        if (nominatedPocket < 6 && ball.pocketIndex != nominatedPocket) isValidTarget = false;

                        if (isValidTarget) {
                            targetIdx = i;
                            break; // Found at least one valid potted ball
                        }
                    }
                }

                if (targetIdx != -1) {
                    if (!gPrediction->guiData.balls[0].onTable) continue;
                    if (!gPrediction->guiData.balls[8].onTable && myclass != Ball::Classification::EIGHT_BALL) continue;

                    auto firstHit = gPrediction->guiData.collision.firstHitBall;
                    if (!firstHit) continue;
                    
                    if (myclass == Ball::Classification::ANY) {
                        if (firstHit->classification == Ball::Classification::EIGHT_BALL) continue;
                    } else if (firstHit->classification != myclass) continue;

                    isPotentiallyValid = true;
                    // Store candidate info
                    g_CurrentCandidate.idx = targetIdx;
                    g_CurrentCandidate.angle = angle;
                    g_CurrentCandidate.power = power;
                    g_CurrentCandidate.pocketIndex = gPrediction->guiData.balls[targetIdx].pocketIndex;
                }

                if (isPotentiallyValid) {
                    LOGI("AutoPlaySlow: Found shot at angle %f power %f", angle, power);
                    foundShot = true;
                    Shoot(angle, power);
                    // Do not reset scanning here, so we can resume if this shot fails
                    break;
                }
            }

            if (foundShot) break;
        }

        if (!foundShot && currentScanAngle >= maxAngle) {
            LOGI("AutoPlaySlow: Finished scan, nothing found.");
            isScanning = false;
            currentScanAngle = 0.0;
            state = IDLE;
        }
    }
    
    void ScanFast(double angleStep = 0.1f) {
        if (g_CurrentCandidate.idx != -1) return;
        if (gPrediction->guiData.balls[0].initialPosition == lastFailedCuePos) return;

        double startingAngle = sharedGameManager.mVisualCue().mVisualGuide().mAimAngle();
        
        gPrediction->determineShotResult(true, startingAngle);
        std::vector<int> startingPottedBalls;
        for (int i = 0; i < gPrediction->guiData.ballsCount; i++) {
            Prediction::Ball& ball = gPrediction->guiData.balls[i];
            if (ball.originalOnTable && !ball.onTable) {
                startingPottedBalls.push_back(i);
            }
        }
        
        Ball::Classification myclass = sharedGameManager.getPlayerClassification();
        uint nominatedPocket = sharedGameManager.getNominatedPocket();
        
        std::vector<Candidate> candidates;
        
        auto pockets = getPockets();
        auto& cueBall = gPrediction->guiData.balls[0];
        
        // Identify candidate shots
        bool bFoundLowestNumberedBall = false;
        int iFoundLowestNumberedBall = -1;
        bool isNineBallGame = myclass == Ball::Classification::NINE_BALL_RULE;
        for (int i = 1; i < gPrediction->guiData.ballsCount; i++) { // ball[0](cue ball) never a candidate
            if (isNineBallGame && bFoundLowestNumberedBall) break;

            auto& ball = gPrediction->guiData.balls[i];
            if (!ball.originalOnTable) continue;

            if (!bFoundLowestNumberedBall) {
                bFoundLowestNumberedBall = true;
                iFoundLowestNumberedBall = i;
            }

            if (!isNineBallGame) {
                bool isACandidate = myclass == Ball::Classification::ANY ? ball.classification != Ball::Classification::EIGHT_BALL : ball.classification == myclass;
                if (!isACandidate) continue;
            }

            for (int pocketIdx = 0; pocketIdx < pockets.size(); pocketIdx++) {
                if (nominatedPocket < 6 && pocketIdx != nominatedPocket) continue;

                Point2D pocket = pockets[pocketIdx];
                Point2D toPocket = pocket - ball.initialPosition;
                double distTargetToPocket = sqrt(toPocket.square());
                if (distTargetToPocket < 0.1) continue;
                
                Point2D direction = toPocket * (1.0 / distTargetToPocket);
                Point2D ghostBallPos = ball.initialPosition - direction * (2.0 * BALL_RADIUS);
                Point2D shotLine = ghostBallPos - cueBall.initialPosition;
                double distCueToTarget = sqrt(shotLine.square());
                double angle = atan2(shotLine.y, shotLine.x);
                
                if (angle < 0) angle += 2 * M_PI;
                
                double score = distCueToTarget + distTargetToPocket;
                
                // Calculate minimum velocity needed to cover the distance
                // v^2 = u^2 + 2as -> u = sqrt(2 * a * s)
                // We use the sliding friction constant from Prediction.h: 196.0
                // _frictionProperties._velocityReductionSlidingFactor
                constexpr double slidingDeceleration = 196.0;
                double requiredVelocity = sqrt(2.0 * slidingDeceleration * score);
                
                // Since shotPower maps 1:1 to velocity in Prediction.h, power IS requiredVelocity.
                 double power = requiredVelocity;
                 
                if (power > 666.0) power = 666.0;
                // if (power < 150.0) power = 150.0; // Removed arbitrary floor, let physics decide
                
                candidates.push_back({i, angle, score, pocketIdx, power});
            }
        }
        
        std::sort(candidates.begin(), candidates.end());
        
        // TODO: more scans around the candidate angle while target ball is hit
        // more power scan
        bool foundShot = false;
        for (const auto& cand : candidates) {
            double angle = NumberUtils::normalizeDoublePrecision(normalizeAngle(cand.angle));
            gPrediction->determineShotResult(true, angle, cand.power, sharedGameManager.getShotSpin(), cand);
            if (!gPrediction->firstHitIsTarget) continue;

            if (!gPrediction->guiData.balls[0].onTable) continue; // cue ball should not be pocketed

            if (myclass == Ball::Classification::NINE_BALL_RULE) {
                auto firstHit = gPrediction->guiData.collision.firstHitBall;
                if (!firstHit) continue;
                // Must hit the target ball (which is the lowest numbered ball) first
                if (firstHit->index != cand.idx) continue;

                int bestPottedIdx = -1;
                for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                    auto& ball = gPrediction->guiData.balls[i];
                    if (ball.originalOnTable && !ball.onTable) {
                        if (nominatedPocket < 6 && ball.pocketIndex != nominatedPocket) continue;
                        
                        if (i == 9) { bestPottedIdx = 9; break; }
                        if (bestPottedIdx == -1 || i == cand.idx) bestPottedIdx = i;
                    }
                }

                if (bestPottedIdx == -1) continue;
                int effectiveTargetIdx = bestPottedIdx;

                if (nominatedPocket < 6 && gPrediction->guiData.balls[effectiveTargetIdx].pocketIndex != nominatedPocket) continue;

                LOGI("AutoPlay: 9ball: Found good angle %f with power %f", angle, cand.power);
                g_CurrentCandidate = cand;
                g_CurrentCandidate.idx = effectiveTargetIdx;
                g_CurrentCandidate.pocketIndex = gPrediction->guiData.balls[effectiveTargetIdx].pocketIndex;

                foundShot = true;
                Shoot(angle, cand.power);
                break;
            }

            if (gPrediction->guiData.balls[cand.idx].onTable) continue; // target ball was not potted
            if (gPrediction->guiData.balls[cand.idx].pocketIndex != cand.pocketIndex) continue; // target ball did not go into target pocket

            std::vector<int> currentPottedBalls;
            bool isAngleGood = false;
            for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                Prediction::Ball& ball = gPrediction->guiData.balls[i];
                bool match = (myclass == Ball::Classification::ANY)
                    ? (ball.classification != Ball::Classification::CUE_BALL && ball.classification != Ball::Classification::EIGHT_BALL)
                    : (ball.classification == myclass);

                if (match && ball.originalOnTable && !ball.onTable) {
                    currentPottedBalls.push_back(i);
                    isAngleGood = true;
                }
            }

            if (isAngleGood && gPrediction->guiData.collision.firstHitBall) {
                 auto firstHit = gPrediction->guiData.collision.firstHitBall;
                 if (myclass != Ball::Classification::ANY && firstHit->classification != myclass) isAngleGood = false;
                 else if (myclass == Ball::Classification::ANY && firstHit->classification == Ball::Classification::EIGHT_BALL) isAngleGood = false;
            }

            auto& cueBallRef = gPrediction->guiData.balls[0];
            if (isAngleGood && cueBallRef.originalOnTable && !cueBallRef.onTable) isAngleGood = false;
            
            auto& eightBallRef = gPrediction->guiData.balls[8];
            bool isEightBallPotted = eightBallRef.originalOnTable && !eightBallRef.onTable;
            if (isAngleGood && isEightBallPotted && myclass != Ball::Classification::EIGHT_BALL) isAngleGood = false;
            
            // if (!currentPottedBalls.empty() && startingPottedBalls != currentPottedBalls && isAngleGood) {
            if (isAngleGood) {
                LOGI("AutoPlay: Found good angle %f with power %f", angle, cand.power);
                g_CurrentCandidate = cand;
                foundShot = true;
                Shoot(angle, cand.power);
                break;
            }
        }

        if (!foundShot) {
            lastFailedCuePos = cueBall.initialPosition;
            LOGI("AutoPlay: No good angle found after smart scan.");
            scan = SLOW;
        }
    }

    // DrawToggleButton is defined in draw.h and called from DrawESP
    // Forward declaration so Update() can still call it directly if needed
    // (currently called from DrawESP directly in both in-game and cancel modes)

    bool isAnimationActive() {
        auto visualCue = sharedGameManager.mVisualCue();
        if (!visualCue) return true;
        
        auto _powerBarView = F(ptr, visualCue + 0x510);
        if (!_powerBarView) return true;

        auto activeAction = M(ptr, libmain + 0x2de6f30, ptr)(_powerBarView); // CCAction getActiveAction
        if (activeAction) {
            // auto tag = F(uint, activeAction + 0x18); // 668 hiding 667 showing
            // LOGI("tag %u %d %p", tag, tag, tag);
            return true;
        }

        return false;
    }
    
    void Update() {
        buttonClicker.Update();
        // DrawToggleButton is now called directly from DrawESP in draw.h

        if (isAnimationActive()) return;

        if (!bAutoPlaying || !sharedGameManager.mStateManager().isPlayerTurn()) {
            g_toggleRotAngle = 0.0f;
            g_autoPlayCalculating = false;
            state = IDLE;
            return;
        }

        if (state == IDLE) {
            state = SCANNING;
            scan = FAST;
        } if (state == SCANNING) {
            if (scan == FAST) {
                g_autoPlayCalculating = false;
                ScanFast();
            }
            if (scan == SLOW) {
                g_autoPlayCalculating = true;
                ScanSlow(0.003f);
            }
        } if (state == NOMINATING) {
            g_autoPlayCalculating = false;
            nominationFrameCounter++;
            if (nominationFrameCounter == 10) {
                buttonClicker.Click(GetPocketScreenPos(g_CurrentCandidate.pocketIndex));
            }
            if (nominationFrameCounter > 20 && !buttonClicker.Active) {
                takeShot(pendingShotAngle, pendingShotPower);
                ClearState();
                state = IDLE;
            }
        }

         /*if (bAutoPlaying && sharedGameManager.mStateManager().isPlayerTurn()) {
            if (powerSlider.Active) {
                UpdateTouchSimulation();
                powerSlider.Update();
            } else Start();
        } */

         //if (!bAutoPlaying && powerSlider.Active) powerSlider.Update(); // for TestAutoPlay
    }
};
