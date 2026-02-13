# RFID Puzzle System - User Manual

## Quick Start Guide

This system uses RFID cards in a specific order to unlock. Think of it like a combination lock, but with cards instead of numbers.

---

## Common Tasks (Most Frequent First)

### 1. **Writing New Cards** (Programming Mode)
*Use this when you need to create new puzzle cards or replace lost ones.*

**Steps:**
1. **Press the button once** (short press, less than 4 seconds)
   - The LED strip will light up **PURPLE** and sweep across
   - You'll hear a two-tone beep
   - The relay will click (unlocking the door as a safety feature)

2. **Choose the number for your card** (0-9)
   - Press the button repeatedly to increase the number
   - The LED strip shows how many LEDs are lit = the number
   - Example: 3 LEDs lit = number 3
   - Each button press makes a quick beep

3. **Place your blank card on the reader**
   - The LEDs will rush across like a wave (green)
   - You'll hear 3 ascending beeps
   - **Success!** Your card is now programmed

4. **Repeat** for more cards
   - Press button to change the number
   - Place next card on reader
   - Continue until all cards are written

5. **Exit Programming Mode**
   - Just press the button to enter a new mode or wait
   - The system will eventually return to normal puzzle mode

**Tip:** Write down which card has which number! You'll need to know this for the puzzle.

---

### 2. **Playing the Puzzle** (Normal Use)
*This is how someone solves the puzzle to unlock.*

**Steps:**
1. Place the **first card** on the reader
   - LEDs rush across the strip (cyan wave)
   - Quick beep
   - First 2 LEDs stay lit **GREEN**

2. Place the **second card** on the reader
   - LEDs rush across again
   - Quick beep
   - First 4 LEDs stay lit **GREEN** (2 per card)

3. Continue until all cards are placed
   - The green progress bar fills up as you add cards

4. **If correct:**
   - 🎉 **Rainbow light show** with sparkles
   - **Victory melody** plays (7 ascending notes)
   - **Relay clicks** and unlocks for 3 seconds
   - Door is unlocked!

5. **If wrong:**
   - ❌ **Red strobe lights**
   - **Sad descending melody** (4 notes going down)
   - Green progress bar clears
   - Try again from the beginning

---

### 3. **Changing the Combination** (Set Combo Mode)
*Use this when you want to change which cards unlock the puzzle.*

**Steps:**
1. **Hold the button for 4+ seconds**
   - Keep holding until the LEDs light up **ORANGE**
   - Two-tone beep confirms you're in Set Combo Mode
   - Release the button

2. **Place cards in the NEW order you want**
   - Place card 1, then card 2, then card 3, etc.
   - Each card makes a beep and shows an orange progress bar
   - The yellow LED shows which position you're on

3. **Wait for confirmation**
   - After the last card, you'll see a **rainbow wave** animation
   - **Victory melody** plays
   - New combination is saved automatically!

4. **Test it immediately**
   - System returns to puzzle mode automatically
   - Try your new combination to make sure it works

**Important:** The system remembers the new combination even after power loss!

---

## Understanding What You See & Hear

### LED Colors Mean:
- 🔵 **Blue** = Puzzle Mode (normal - waiting for cards)
- 🟣 **Purple** = Programming Mode (writing cards)
- 🟠 **Orange** = Set Combo Mode (changing the combination)
- 🟢 **Green Progress Bar** = Shows how many cards you've entered
- 🌈 **Rainbow** = Success!
- 🔴 **Red Strobe** = Wrong combination

### LED Animations:
- **Wave/Rush Effect** = Card detected and read successfully
- **Progress Bar (2 LEDs per card)** = Shows your position in the sequence
- **Rainbow Chase** = Celebrating success
- **Red Flashing** = Error or wrong answer

### Buzzer Sounds:
- **Single Beep** (high) = Card read successfully
- **Two-Tone Beep** = Mode changed
- **Quick Beep** (medium) = Number changed in programming mode
- **Ascending 7 Notes** = Success melody (puzzle unlocked!)
- **Descending 4 Notes** = Failure melody (wrong combination)
- **3 Ascending Beeps** = Card written successfully
- **3 Harsh Buzzes** = Card write failed

---

## Troubleshooting

### Problem: Card won't read
**What you see:** Red strobe, harsh buzzing
**Solutions:**
- Make sure card is flat and centered on the reader
- Hold the card still for 1-2 seconds
- Try removing and placing the card again
- Card might be damaged - try a different card

### Problem: Can't enter Programming Mode
**What you see:** Nothing happens when pressing button
**Solutions:**
- Make sure you're in Puzzle Mode (blue LEDs, not orange/purple)
- Press and release the button quickly (less than 4 seconds)
- Check that the button is connected properly

### Problem: Can't write to a card
**What you see:** Red strobe after placing card, harsh buzzing
**Solutions:**
- Card might already be locked or incompatible
- Try a fresh, blank MIFARE Classic card
- Make sure the card stays on the reader during writing
- Check that you're in Programming Mode (purple LEDs)

### Problem: Combination doesn't work
**What you see:** Red strobe, sad melody after entering all cards
**Solutions:**
- Check you're using the right cards in the right order
- Make sure each card was read (you should see/hear confirmation)
- The green progress bar should fill completely
- Try entering the sequence more slowly

### Problem: System doesn't respond at all
**What you see:** No LEDs, no sounds
**Solutions:**
- Check power connection to Arduino
- Make sure all wires are connected
- Try unplugging and replugging power (restart)

### Problem: Relay doesn't click
**What you see:** Correct puzzle solve, but no unlock
**Solutions:**
- Check relay wiring to Pin 5
- Relay might need external power source
- Test in Programming Mode - relay should click when entering mode

---

## Quick Reference Card

| **Action** | **Button Press** | **LED Color** | **What It Does** |
|------------|------------------|---------------|------------------|
| Enter Programming Mode | Short press (<4s) | Purple | Write numbers to cards |
| Enter Set Combo Mode | Hold 4+ seconds | Orange | Change the winning order |
| Increment Number | Quick press (in Program Mode) | Blue gradient | Change number 0-9 |
| Solve Puzzle | Place cards in order | Green progress | Unlock if correct |

---

## Safety Notes

⚠️ **Programming Mode automatically unlocks the relay** - This is a safety feature so you can always access the door by entering Programming Mode, even if you forget the combination.

⚠️ **The combination is saved permanently** - Even if power is lost, the system remembers the last saved combination.

⚠️ **Default Combination** - If you've never set a combination, the default is cards: 1, 2, 3, 4 (in that order).

---

## Tips for Success

✅ **Label your cards!** Write the number on each card with a marker so you know which is which.

✅ **Test after programming** - After writing cards, immediately test them in puzzle mode to confirm they work.

✅ **Keep a backup** - Write down the current combination somewhere safe.

✅ **Be patient with the reader** - Give each card 1-2 seconds on the reader for reliable reading.

✅ **Watch the progress bar** - The green LEDs tell you exactly where you are in the sequence (2 LEDs = 1 card).

---

## Common Scenarios

### Scenario 1: "I need to make cards for a new puzzle"
1. Press button once → Purple lights (Programming Mode)
2. Press button until you see 1 LED lit
3. Place card → Write card #1
4. Press button until you see 2 LEDs lit
5. Place card → Write card #2
6. Repeat for cards 3, 4, etc.
7. Label each card!

### Scenario 2: "I want to change the unlock sequence"
1. Hold button for 5 seconds → Orange lights (Set Combo Mode)
2. Place cards in the NEW order you want
3. Wait for rainbow → New combo saved!
4. Test it immediately

### Scenario 3: "Someone lost a card"
1. Press button once → Purple lights (Programming Mode)
2. Set the number to match the lost card
3. Write a new replacement card
4. The puzzle will work with the new card

### Scenario 4: "I forgot the combination"
1. Press button once → Programming Mode (relay unlocks as bypass)
2. Access granted through the door
3. Then set a new combination you'll remember

---

## Technical Specifications
*(For reference only)*

- **Cards Supported:** MIFARE Classic 1K
- **Maximum Cards in Sequence:** 10 cards
- **Card Numbers:** 0-9
- **Relay Activation Time:** 3 seconds
- **Power Requirements:** 5V DC via Arduino Nano
- **Operating Temperature:** Standard indoor conditions

---

## Need Help?

If you're still having trouble, check:
1. Is the Arduino powered on? (LED strip should show rainbow on startup)
2. Are all connections secure?
3. Are you using MIFARE Classic cards?
4. Have you tried restarting the system?

For technical support, contact your system administrator.

---

*Last Updated: 2026*
*Version: Production v1.0*
