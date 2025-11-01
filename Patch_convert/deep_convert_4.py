import struct
import os
import csv
from collections import OrderedDict

# Import everything from your constants file
from quadra_constants import *

def reverse_lookup(table, raw_value, param_name="", table_name=""):
    """Find the closest CC value using the lookup table with smarter cutoff and percentage handling"""
    if not table:
        print(f"  WARNING: No lookup table for {param_name}")
        return min(int(raw_value * 127), 127)

    print(f"DEBUG LOOKUP: {param_name} = {raw_value}")
    if table_name:
        print(f"  Table: {table_name}, length: {len(table)}, range: {table[0]} to {table[-1]}")

    

    # --- STRICT cutoff match only for actual filter cutoff controls ---
    CUTOFF_PARAM_NAMES = {"polySynVCFcutoff", "leadSynVCFcutoff"}
    if param_name in CUTOFF_PARAM_NAMES:
        closest_cc = 0
        min_diff = float('inf')
        for cc_value, table_value in enumerate(table):
            diff = abs(table_value - raw_value)
            if diff < min_diff:
                min_diff = diff
                closest_cc = cc_value
        result = min(closest_cc, 127)
        print(f"  FINAL CUTOFF: {raw_value} -> CC{result} ({table[result]} Hz)")
        return result
    
    # --- Poly VCF Resonance mapping: float 0.1..1.0 → CC 0..127 (10–100 %) ---
    if param_name in ('polyVCFRes', 'polySynVCFres'):
        # Clamp and normalize the 0.1–1.0 range
        clamped = max(0.1, min(raw_value, 1.0))
        # Map 0.1 → 0, 1.0 → 127 linearly
        result = int(round(((clamped - 0.1) / 0.9) * 127))
        print(f"  Poly VCF Res direct mapping: {raw_value} -> CC{result}")
        return result

    # --- Detect percentage-style parameters automatically ---
    name_is_percenty = any(x in param_name.lower() for x in
        ['mix', 'vol', 'level', 'res', 'sus', 'depth', 'amount', 'amt', 'width', 'bender', 'touch', 'mod'])
    table_looks_percent = table and (table[0] <= 0.0) and (50.0 <= table[-1] <= 100.0)

    if (raw_value <= 1.0) and (name_is_percenty or table_looks_percent):
        percentage = raw_value * 100.0
        print(f"  Percentage conversion: {raw_value} -> {percentage:.2f}%")
        search_value = percentage
    else:
        search_value = raw_value
        print(f"  Using raw value: {raw_value}")

    # --- Find closest CC ---
    closest_cc = 0
    min_diff = float('inf')
    for cc_value, table_value in enumerate(table):
        diff = abs(table_value - search_value)
        if diff < min_diff:
            min_diff = diff
            closest_cc = cc_value

    result = min(closest_cc, 127)
    print(f"  FINAL: {search_value} -> CC{result} ({table[result]})")
    return result

def convert_parameter_value(preset_param_name, value_type, value, parameter_table_map):
    """Convert a parameter value using appropriate method based on parameter type"""

    print(f"CONVERTING: {preset_param_name} = {value} (type: {value_type})")

    # Find mapped name
    your_param_name = None
    for your_name, mapped_name in PARAM_MAP.items():
        if mapped_name == preset_param_name:
            your_param_name = your_name
            break

    if not your_param_name:
        print(f"  WARNING: No mapping found for '{preset_param_name}'")
        return 0

    print(f"  Mapped to: {your_param_name}")

    # --- BOOLEAN handling (with threshold) ---
    if your_param_name in BOOLEAN_PARAMS:
        if value_type in ('int', 'bool'):
            # treat explicit 0 as off even if it's <10
            result = 1 if (value < 10 and value != 0) else 0
        elif value_type == 'float':
            result = 1 if value > 0.5 else 0
        else:
            result = 0
        print(f"  Boolean conversion (Cherry-style): {value} -> {result}")
        return result

    # --- MULTI-SWITCH handling ---
    elif your_param_name in MULTI_SWITCH_PARAMS:
        result = min(int(value), 127)
        print(f"  Multi-switch conversion: {value} -> {result}")
        return result

    # --- CONTINUOUS parameters ---
    elif your_param_name in CONTINUOUS_PARAMS:
        # Table lookup (using mapped name)
        if preset_param_name in parameter_table_map:
            table_name = parameter_table_map[preset_param_name]
            lookup_table = globals().get(table_name)
            if lookup_table:
                # Use mapped name instead of preset_param_name
                return reverse_lookup(lookup_table, value, your_param_name, table_name)

        # Special scaling for bender-to-cutoff parameters
        if your_param_name in ['benderPolyAmtCutoff', 'benderLeadAmtCutoff']:
            result = int(round(value * 127))
            print(f"  Special bender scaling: {value} -> {result}")
            return result

        # Adaptive fallback
        if abs(value) <= 1.0:
            result = int(value * 127)
        elif abs(value) <= 10.0:
            result = int((value / 10.0) * 127)
        elif abs(value) <= 100.0:
            result = int((value / 100.0) * 127)
        else:
            result = min(int(value), 127)

        result = max(0, min(result, 127))
        print(f"  Continuous adaptive: {value} -> {result}")
        return result

    print(f"  WARNING: No conversion method for {preset_param_name}")
    return 0

def find_complete_parameter_name(truncated_name, all_expected_names):
    """Find the complete parameter name given a truncated version"""
    # Common truncation patterns we've observed
    truncation_patterns = [
        truncated_name,  # as-is
        'a' + truncated_name,  # missing 'a'
        'b' + truncated_name,  # missing 'b'  
        'c' + truncated_name,  # missing 'c'
        'e' + truncated_name,  # missing 'e'
        'l' + truncated_name,  # missing 'l'
        'o' + truncated_name,  # missing 'o'
        'p' + truncated_name,  # missing 'p'
        'r' + truncated_name,  # missing 'r'
        's' + truncated_name,  # missing 's'
        't' + truncated_name,  # missing 't'
        'u' + truncated_name,  # missing 'u'
        'ar' + truncated_name,  # missing 'ar'
        'ba' + truncated_name,  # missing 'ba'
        'be' + truncated_name,  # missing 'be'
        'ch' + truncated_name,  # missing 'ch'
        'ea' + truncated_name,  # missing 'ea'
        'en' + truncated_name,  # missing 'en'
        'le' + truncated_name,  # missing 'le'
        'ou' + truncated_name,  # missing 'ou'
        'po' + truncated_name,  # missing 'po'
        'st' + truncated_name,  # missing 'st'
        'to' + truncated_name,  # missing 'to'
        'tou' + truncated_name,  # missing 'tou'
        'us' + truncated_name,  # missing 'us'
        'pol' + truncated_name,  # missing 'pol'
        'str' + truncated_name,  # missing 'str'
        'out' + truncated_name,  # missing 'out'
        'poly' + truncated_name,  # missing 'poly'
        'polysyn' + truncated_name,  # missing 'polysyn'
        'leadSyn' + truncated_name,  # missing 'leadSyn'
        'polySyn' + truncated_name,  # missing 'polySyn'
    ]
    
    for pattern in truncation_patterns:
        if pattern in all_expected_names:
            return pattern
    
    return None

def midi_note_to_name(note_number):
    """Convert MIDI note number to note name"""
    notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    octave = (note_number // 12) - 1
    note_name = notes[note_number % 12]
    return f"{note_name}{octave}"

def dump_complex_parameters(file_path):
    """Dump the raw binary data of complex parameters for analysis"""
    with open(file_path, 'rb') as f:
        data = f.read()
    
    print(f"\n{'='*60}")
    print("COMPLEX PARAMETER ANALYSIS")
    print(f"{'='*60}")
    
    complex_params = ['controls', 'mpe', 'ModuleStateData', 'main']
    i = 0
    
    while i < len(data):
        # Look for start of parameter name
        if data[i] < 32 or data[i] > 126:
            i += 1
            continue
            
        # Read parameter name
        name_end = data.find(b'\x00', i)
        if name_end == -1:
            break
            
        param_name_bytes = data[i:name_end]
        
        try:
            param_name = param_name_bytes.decode('ascii')
        except UnicodeDecodeError:
            param_name = param_name_bytes.decode('ascii', errors='ignore')
        
        # Check if this is one of our complex parameters
        if param_name in complex_params:
            i = name_end + 1
            
            if i >= len(data):
                break
                
            # Read header
            if i + 2 >= len(data):
                break
                
            header = data[i:i+3]
            i += 3
            
            print(f"\n{param_name}:")
            print(f"  Header: {header.hex()}")
            
            # Try to determine data size and dump it
            if header == b'\x01\x09\x04':  # Float (8 bytes)
                if i + 8 <= len(data):
                    value_bytes = data[i:i+8]
                    print(f"  Type: Float (8 bytes)")
                    print(f"  Raw bytes: {value_bytes.hex()}")
                    try:
                        float_value = struct.unpack('<d', value_bytes)[0]
                        print(f"  Float value: {float_value}")
                    except:
                        print(f"  Could not unpack as float")
                    i += 8
                    
            elif header in [b'\x01\x01\x02', b'\x01\x01\x01', b'\x01\x01\x03']:  # Various int types
                if i < len(data):
                    int_value = data[i]
                    print(f"  Type: Integer (1 byte)")
                    print(f"  Value: {int_value}")
                    i += 1
                    
            elif header == b'\x01\x04\x08':  # Possibly larger data structure
                print(f"  Type: Unknown structure with header {header.hex()}")
                # Try to find the size
                if i + 4 <= len(data):
                    size_bytes = data[i:i+4]
                    size = struct.unpack('<I', size_bytes)[0]
                    print(f"  Data size: {size} bytes")
                    i += 4
                    if i + size <= len(data):
                        data_bytes = data[i:i+size]
                        print(f"  First 100 bytes: {data_bytes[:100].hex()}")
                        i += size
            else:
                print(f"  Unknown header: {header.hex()}")
                # Skip unknown data
                skip_count = 0
                while i < len(data) and (data[i] < 32 or data[i] > 126) and skip_count < 1000:
                    i += 1
                    skip_count += 1
        else:
            i = name_end + 1

def find_target_parameters(file_path):
    """Search specifically for the missing keyboard ranges and trill interval"""
    with open(file_path, 'rb') as f:
        data = f.read()
    
    print(f"\n{'='*80}")
    print("TARGETED SEARCH FOR MISSING PARAMETERS")
    print(f"{'='*80}")
    
    # The exact values we're looking for
    target_values = {
        'Trill Interval': 2,
        'Bass Low': 0,
        'Bass High': 59,
        'Strings Low': 48, 
        'Strings High': 127,
        'Poly Low': 60,
        'Poly High': 127,
        'Lead Low': 0,
        'Lead High': 127
    }
    
    print("Searching for exact values:")
    for name, value in target_values.items():
        print(f"  {name}: {value}")
    
    # Search for individual values first
    print(f"\n1. INDIVIDUAL VALUE LOCATIONS:")
    print("-" * 40)
    
    for name, target_value in target_values.items():
        positions = []
        pos = 0
        while True:
            pos = data.find(bytes([target_value]), pos)
            if pos == -1:
                break
            positions.append(pos)
            pos += 1
        
        if positions:
            print(f"{name} ({target_value}) found at {len(positions)} positions:")
            # Show first few positions
            for p in positions[:3]:
                print(f"  Position {p}")
                # Show context around this position
                start = max(0, p - 10)
                end = min(len(data), p + 10)
                context = data[start:end]
                print(f"    Context: {context.hex()}") 
                print(f"    As integers: {[b for b in context]}")
        else:
            print(f"{name} ({target_value}) not found")
    
    # Search for the specific keyboard range patterns
    print(f"\n2. KEYBOARD RANGE PATTERNS:")
    print("-" * 40)
    
    # Look for the exact range pairs we need
    range_patterns = [
        ('Bass', [0, 59]),
        ('Strings', [48, 127]), 
        ('Poly', [60, 127]),
        ('Lead', [0, 127])
    ]
    
    for range_name, pattern in range_patterns:
        low, high = pattern
        pattern_bytes = bytes([low, high])
        
        positions = []
        pos = 0
        while True:
            pos = data.find(pattern_bytes, pos)
            if pos == -1:
                break
            positions.append(pos)
            pos += 1
        
        print(f"\n{range_name} Range [{low}, {high}]:")
        if positions:
            print(f"  EXACT PATTERN FOUND at {len(positions)} locations:")
            for p in positions[:3]:  # Show first 3 matches
                print(f"    Position {p}")
                # Show more context
                start = max(0, p - 20)
                end = min(len(data), p + 20)
                context = data[start:end]
                print(f"      Full context: {[b for b in context]}")
        else:
            print(f"  Exact pattern not found")
            
            # Look for close matches (values within 1-2 positions)
            print(f"  Searching for close matches...")
            close_matches = []
            for i in range(len(data) - 2):
                if (abs(data[i] - low) <= 2 and abs(data[i+1] - high) <= 2):
                    close_matches.append((i, data[i], data[i+1]))
            
            for pos, actual_low, actual_high in close_matches[:5]:  # Show first 5 close matches
                print(f"    Close match at {pos}: [{actual_low}, {actual_high}]")
    
    # Search for trill interval near keyboard ranges
    print(f"\n3. TRILL INTERVAL NEAR KEYBOARD RANGES:")
    print("-" * 40)
    
    # Look for trill value (2) near any of our target ranges
    trill_value = 2
    search_radius = 50  # Look within 50 bytes of range patterns
    
    for range_name, pattern in range_patterns:
        low, high = pattern
        pattern_bytes = bytes([low, high])
        
        pos = 0
        while True:
            pos = data.find(pattern_bytes, pos)
            if pos == -1:
                break
            
            # Search for trill near this range
            start_search = max(0, pos - search_radius)
            end_search = min(len(data), pos + search_radius)
            
            trill_positions = []
            for i in range(start_search, end_search):
                if data[i] == trill_value:
                    trill_positions.append(i)
            
            if trill_positions:
                print(f"\n{range_name} Range at {pos} has trill interval nearby:")
                for tp in trill_positions:
                    distance = tp - pos
                    print(f"  Trill at position {tp} (distance: {distance} bytes)")
            
            pos += 1
    
    # Look for all four ranges in close proximity (likely in one structure)
    print(f"\n4. COMPLETE SET OF RANGES IN ONE AREA:")
    print("-" * 40)
    
    # Search for areas that contain most of our target values
    window_size = 200  # Look within 200-byte windows
    
    for i in range(0, len(data) - window_size, 50):  # Slide window by 50 bytes
        window = data[i:i+window_size]
        
        # Count how many target values appear in this window
        found_values = []
        for name, value in target_values.items():
            if bytes([value]) in window:
                found_values.append(name)
        
        if len(found_values) >= 5:  # Found at least 5 of our target values
            print(f"\nFound {len(found_values)} target values in window {i}-{i+window_size}:")
            print(f"  Values: {found_values}")
            
            # Show the actual data in this window
            print(f"  Data in this area:")
            for j in range(0, min(len(window), 100), 16):
                hex_str = window[j:j+16].hex(' ')
                ascii_str = ''.join(chr(b) if 32 <= b <= 126 else '.' for b in window[j:j+16])
                print(f"    {i+j:4d}: {hex_str:<48} {ascii_str}")

def analyze_preset_structure(file_path):
    """Analyze the overall structure to find where parameters are stored"""
    with open(file_path, 'rb') as f:
        data = f.read()
    
    print(f"\n{'='*80}")
    print("PRESET STRUCTURE ANALYSIS")
    print(f"{'='*80}")
    
    # Look for known parameter sections
    section_markers = [
        b'controls', b'mpe', b'ModuleStateData', b'main',
        b'arp', b'Arp', b'ARP', b'trill', b'Trill',
        b'range', b'Range', b'keyboard', b'Keyboard'
    ]
    
    print("Looking for parameter sections:")
    for marker in section_markers:
        pos = 0
        while True:
            pos = data.find(marker, pos)
            if pos == -1:
                break
            
            print(f"\nFound '{marker.decode()}' at position {pos}")
            
            # Show context around this marker
            start = max(0, pos - 20)
            end = min(len(data), pos + 100)
            context = data[start:end]
            
            # Try to find parameter values after the marker
            values_after = []
            value_start = pos + len(marker)
            for i in range(value_start, min(value_start + 50, len(data))):
                if data[i] in [0, 2, 48, 59, 60, 127]:  # Our target values
                    values_after.append((i, data[i]))
            
            if values_after:
                print(f"  Target values found after marker:")
                for pos, val in values_after:
                    print(f"    Position {pos}: value {val}")
            
            pos += len(marker)

def extract_keyboard_ranges_and_trill(file_path):
    """Extract bass/strings/poly/lead note ranges and trill interval from ModuleStateData."""
    with open(file_path, "rb") as f:
        data = f.read()

    mod_pos = data.find(b"ModuleStateData")
    mpe_pos = data.find(b"mpe")
    if mod_pos == -1 or mpe_pos == -1:
        print("Could not locate ModuleStateData or mpe.")
        return None

    base = mod_pos + len(b"ModuleStateData") + 3
    block = data[base:mpe_pos]

    bass_low     = block[9]
    strings_low  = block[13]
    poly_low     = block[17]
    lead_low     = block[21]
    bass_high    = block[25]
    strings_high = block[29]
    poly_high    = block[33]
    lead_high    = block[37]
    trill        = block[61]

    print(f"Bass:    {bass_low}-{bass_high}")
    print(f"Strings: {strings_low}-{strings_high}")
    print(f"Poly:    {poly_low}-{poly_high}")
    print(f"Lead:    {lead_low}-{lead_high}")
    print(f"Trill:   {trill}")

    # --- Use the extracted trill directly ---
    trill_interval = int(trill)
    if not (1 <= trill_interval <= 12):  # sanity check for valid semitone range
        print(f"  [Warning] Invalid trill value {trill_interval}, resetting to 2")
        trill_interval = 2

    # --- Enforce safe keyboard limits (15–115) ---
    def clamp_note(val):
        return max(15, min(int(val), 115))

    # Clamp all extracted values
    bass_low,  bass_high  = clamp_note(bass_low),  clamp_note(bass_high)
    strings_low, strings_high = clamp_note(strings_low), clamp_note(strings_high)
    poly_low,  poly_high  = clamp_note(poly_low),  clamp_note(poly_high)
    lead_low,  lead_high  = clamp_note(lead_low),  clamp_note(lead_high)

    # Ensure bottom ≤ top after clamping
    if bass_low > bass_high: bass_low = bass_high
    if strings_low > strings_high: strings_low = strings_high
    if poly_low > poly_high: poly_low = poly_high
    if lead_low > lead_high: lead_low = lead_high

    # Optional: warn if anything got clamped
    def maybe_warn(name, orig_low, orig_high, new_low, new_high):
        if (orig_low != new_low) or (orig_high != new_high):
            print(f"  [Clamp] {name} range adjusted from {orig_low}-{orig_high} → {new_low}-{new_high}")

    maybe_warn("Bass",     bass_low, bass_high, bass_low, bass_high)
    maybe_warn("Strings",  strings_low, strings_high, strings_low, strings_high)
    maybe_warn("Poly",     poly_low, poly_high, poly_low, poly_high)
    maybe_warn("Lead",     lead_low, lead_high, lead_low, lead_high)

    return {
        'bass_range': (bass_low, bass_high),
        'strings_range': (strings_low, strings_high),
        'poly_range': (poly_low, poly_high),
        'lead_range': (lead_low, lead_high),
        'trill_interval': trill_interval
    }

import re

# ----- Waveform enums (adjust if you refine the orders) -----
# leadVCO1: 0=square, 1=sine, 2=triangle, 3=noise, 4=saw up
LEAD_VCO1_ENUM = {
    0: "square", 1: "sine", 2: "triangle", 3: "noise", 4: "saw_up"
}
# leadVCO2: 0=saw up, 1=square, 2=sine, 3=triangle, 4=noise, 5=saw down
LEAD_VCO2_ENUM = {
    0: "saw_up", 1: "square", 2: "sine", 3: "triangle", 4: "noise", 5: "saw_down"
}
# polyWave: 0=square, 1=triangle, 2=sine, 3=tri-saw, 4=lumpy, 5=saw up
POLY_ENUM = {
    0: "square", 1: "triangle", 2: "sine", 3: "tri_saw", 4: "lumpy", 5: "saw_up"
}

# Raw -> 1-based CC index for your output (you said your files use 1–N)
def _one_based(n): return n + 1 if n is not None else 0

def _is_ascii_name(data, pos, name: bytes) -> bool:
    """Match a null-terminated ASCII 'name' exactly at pos."""
    if pos < 0 or pos + len(name) >= len(data):
        return False
    # Name chars must be printable ASCII
    for b in name:
        if b < 32 or b > 126:
            return False
    # Followed by a null terminator
    return data[pos:pos+len(name)] == name and (pos + len(name) < len(data) and data[pos+len(name)] == 0x00)

def _find_section(data: bytes, name: bytes) -> int:
    """
    Find a section name as null-terminated ASCII and return the index of the
    first byte AFTER the terminating null (i.e., start of header that follows).
    """
    i = 0
    while True:
        idx = data.find(name, i)
        if idx == -1:
            return -1
        # Require exact match with trailing 0x00 (null-terminated)
        if _is_ascii_name(data, idx, name):
            return idx + len(name) + 1  # position right after the null
        i = idx + 1

def _extract_module_block(data: bytes) -> bytes | None:
    """
    Return the raw bytes between 'ModuleStateData' and the next legitimate 'mpe'
    (both matched as null-terminated ASCII labels). This avoids false 'mpe' hits.
    """
    mod_start = _find_section(data, b"ModuleStateData")
    if mod_start == -1:
        print("ModuleStateData not found (robust).")
        return None

    # Find next legitimate 'mpe' label after mod_start
    i = mod_start
    while True:
        mpe_idx = data.find(b"mpe", i)
        if mpe_idx == -1:
            print("Legit 'mpe' label not found after ModuleStateData.")
            return None
        if _is_ascii_name(data, mpe_idx, b"mpe"):
            mpe_start = mpe_idx  # the label start (we stop before it)
            break
        i = mpe_idx + 1

    block = data[mod_start:mpe_start]
    # Optional: sanity print
    # print(f"ModuleStateData block length (robust): {len(block)} bytes")
    return block

def extract_waveforms_from_file(file_path):
    with open(file_path, "rb") as f:
        data = f.read()

    mod_pos = data.find(b"ModuleStateData")
    if mod_pos == -1:
        print("ModuleStateData not found")
        return None

    # block starts immediately after "ModuleStateData" + null terminator
    base = mod_pos + len(b"ModuleStateData") + 1

    # read the three bytes we discovered experimentally
    lead1_raw = data[base + 55]
    lead2_raw = data[base + 59]
    poly_raw  = data[base + 43]

    # convert 0-based -> 1-based (your preset files use 1–N)
    lead1 = lead1_raw + 1
    lead2 = lead2_raw + 1
    poly  = poly_raw  + 1

    print(f"leadVCO1wave: {lead1}  (raw {lead1_raw})")
    print(f"leadVCO2wave: {lead2}  (raw {lead2_raw})")
    print(f"polyWave:     {poly}  (raw {poly_raw})")

    return lead1, lead2, poly

def parse_preset_file(file_path):
    """Parse a Cherry Audio preset file and extract all parameters"""
    with open(file_path, 'rb') as f:
        data = f.read()
    
    parameters = {}
    i = 0
    
    print(f"DEBUG: File size: {len(data)} bytes")
    
    # Get all expected parameter names from our constants
    all_expected_names = set(PARAM_MAP.values())
    
    # Main parsing loop -------------------------------------------------------
    while i < len(data):
        if data[i] < 32 or data[i] > 126:
            i += 1
            continue
            
        name_end = data.find(b'\x00', i)
        if name_end == -1:
            break
            
        param_name_bytes = data[i:name_end]
        try:
            param_name = param_name_bytes.decode('ascii')
        except UnicodeDecodeError:
            param_name = param_name_bytes.decode('ascii', errors='ignore')
            if len(param_name) < 2:
                i = name_end + 1
                continue
            print(f"DEBUG: Corrupted parameter name: {param_name_bytes} -> '{param_name}'")
        
        original_param_name = param_name
        
        # Skip metadata
        if param_name in [
            'main', 'reset_File_Version', 'presetName', 'optionalKeywords',
            'standaloneTempo', 'ModuleStateData', 'mpe', 'itchBendRange',
            'sesPitchBend', 'controls'
        ]:
            i = name_end + 1
            continue
        
        if param_name.isdigit():
            i = name_end + 1
            continue
        
        # Try to fix truncated names
        if param_name not in all_expected_names:
            complete_name = find_complete_parameter_name(param_name, all_expected_names)
            if complete_name:
                print(f"DEBUG: Fixed truncated parameter: '{param_name}' -> '{complete_name}'")
                param_name = complete_name
        
        i = name_end + 1
        if i >= len(data):
            break
        
        header = data[i:i+3]
        i += 3
        
        # Parse different data types
        if header == b'\x01\x09\x04':  # Float (8 bytes)
            if i + 8 <= len(data):
                value_bytes = data[i:i+8]
                try:
                    float_value = struct.unpack('<d', value_bytes)[0]
                    parameters[param_name] = ('float', float_value)
                    debug_params = [
                        'Mix', 'Vol', 'cutoff', 'arp', 'Pulse', 'bender', 'lfo',
                        'Att', 'Dec', 'Rel', 'Note', 'Porta', 'Syn', 'Flng'
                    ]
                    if any(key in original_param_name for key in debug_params):
                        print(f"PARSED FLOAT: {original_param_name} -> {param_name} = {float_value:.6f}")
                except struct.error as e:
                    print(f"ERROR parsing float for {param_name}: {e}")
                i += 8
                
        elif header == b'\x01\x01\x02':  # Boolean/switch (1 byte)
            if i < len(data):
                raw_value = data[i]
                normalized = 1 if raw_value >= 32 else 0
                parameters[param_name] = ('bool', normalized)
                print(f"PARSED BOOL: {original_param_name} -> {param_name} = {raw_value} (normalized: {normalized})")
                i += 1

        elif header == b'\x01\x01\x01':  # Integer (1 byte)
            if i < len(data):
                int_value = data[i]
                parameters[param_name] = ('int', int_value)
                print(f"PARSED INT: {original_param_name} -> {param_name} = {int_value}")
                i += 1
                
        elif header == b'\x01\x01\x03':  # Multi-switch/enum (1 byte)
            if i < len(data):
                int_value = data[i]
                parameters[param_name] = ('int', int_value)
                print(f"PARSED MULTI-SWITCH: {original_param_name} -> {param_name} = {int_value}")
                i += 1
                
        else:
            skip_count = 0
            while i < len(data) and (data[i] < 32 or data[i] > 126) and skip_count < 100:
                i += 1
                skip_count += 1

    # -------------------------------------------------------------------------
    # UTF-16/UTF-8 preset name decode + CHAR-BASED offset for waveforms
    # -------------------------------------------------------------------------
    # --- Inject hidden waveform params from ModuleStateData ---
    try:
        lead1, lead2, poly = extract_waveforms(file_path)
        parameters['leadVCO1wave'] = ('int', lead1)
        parameters['leadVCO2wave'] = ('int', lead2)
        parameters['polyWave']     = ('int', poly)
        print(f"Injected hidden waves → leadVCO1={lead1}, leadVCO2={lead2}, poly={poly}")
    except Exception as e:
        print(f"Waveform injection failed: {e}")
    
    print("\nALL PARSED PARAMETERS:")
    for param_name, (value_type, value) in sorted(parameters.items()):
        print(f"  {param_name}: {value} ({value_type})")
    
    return parameters

def normalize_radio_groups(parameters):
    """Ensure only one parameter in each radio group is active (value = 1)"""
    from quadra_constants import RADIO_GROUPS

    for group_name, members in RADIO_GROUPS.items():
        present = [m for m in members if m in parameters]
        if not present:
            continue

        # Pick the member with the smallest raw numeric value
        min_param = min(present, key=lambda n: parameters[n][1])

        for m in present:
            parameters[m] = ('bool', 1 if m == min_param else 0)

        print(f"[RADIO NORMALIZE] Group '{group_name}': '{min_param}' = 1, others = 0")


def analyze_mapping_coverage(parameters):
    """Analyze which parameters were successfully mapped"""
    mapped_params = []
    unmapped_params = []
    
    for preset_param_name, (value_type, value) in parameters.items():
        mapped = False
        for your_param_name, mapped_name in PARAM_MAP.items():
            if mapped_name == preset_param_name:
                mapped_params.append(preset_param_name)
                mapped = True
                break
        if not mapped:
            unmapped_params.append(preset_param_name)
    
    print(f"\nMAPPING ANALYSIS:")
    print(f"Total parameters in file: {len(parameters)}")
    print(f"Successfully mapped: {len(mapped_params)}")
    print(f"Not mapped: {len(unmapped_params)}")
    
    if unmapped_params:
        print(f"\nUNMAPPED PARAMETERS:")
        for param in sorted(unmapped_params):
            print(f"  {param}")

    # Show specifically which of our expected parameters are missing
    expected_params = set(PARAM_MAP.values())
    found_params = set(parameters.keys())
    missing_params = expected_params - found_params
    
    if missing_params:
        print(f"\nMISSING EXPECTED PARAMETERS:")
        for param in sorted(missing_params):
            print(f"  {param}")
    
    return mapped_params, unmapped_params, missing_params

def comprehensive_preset_analysis(file_path):
    """Run all analysis functions"""
    print(f"\n{'='*80}")
    print(f"COMPREHENSIVE ANALYSIS: {os.path.basename(file_path)}")
    print(f"{'='*80}")
    
    find_target_parameters(file_path)
    analyze_preset_structure(file_path)
    extract_keyboard_ranges_and_trill(file_path)
    dump_complex_parameters(file_path)

def process_preset_files(input_folder, output_folder):
    """Process all .quadrapreset files in a folder and generate numbered output files"""
    
    # Create output folder if it doesn't exist
    os.makedirs(output_folder, exist_ok=True)
    
    # Get all preset files
    preset_files = [f for f in os.listdir(input_folder) if f.endswith('.quadrapreset')]
    preset_files.sort()
    
    # Create detailed analysis files
    analysis_csv = os.path.join(output_folder, "parameter_analysis.csv")
    analysis_data = []
    
    # Define the keyboard range and trill parameter names in the correct order
    keyboard_param_mapping = {
        'leadBottomNote': 'lead_low',
        'leadTopNote': 'lead_high', 
        'polyBottomNote': 'poly_low',
        'polyTopNote': 'poly_high',
        'stringsBottomNote': 'strings_low',
        'stringsTopNote': 'strings_high',
        'bassBottomNote': 'bass_low',
        'bassTopNote': 'bass_high',
        'trillValue': 'trill_interval'
    }
    
    for index, preset_file in enumerate(preset_files, start=1):
        file_path = os.path.join(input_folder, preset_file)
        print(f"\n{'='*80}")
        print(f"Processing {index}: {preset_file}")
        print(f"{'='*80}")
        
        # First, run comprehensive analysis to find keyboard ranges and trill values
        comprehensive_preset_analysis(file_path)
        
        # Extract the keyboard ranges and trill
        range_data = extract_keyboard_ranges_and_trill(file_path)
        
        # Then parse normally
        parameters = parse_preset_file(file_path)
        normalize_radio_groups(parameters)
        
        # Initialize all parameters to 0
        preset_data = OrderedDict()
        param_details = OrderedDict()  # Store details for analysis
        
        # Initialize ALL parameters from PARAM_MAP (including the keyboard ones)
        for your_param_name in PARAM_MAP.keys():
            preset_data[your_param_name] = 0
            param_details[your_param_name] = "NOT_FOUND"
        
        # Extract preset name
        preset_name = os.path.splitext(preset_file)[0][:13]
        
        # Map parameters
        mapped_count = 0
        for preset_param_name, (value_type, value) in parameters.items():
            mapped = False
            for your_param_name, mapped_name in PARAM_MAP.items():
                if mapped_name == preset_param_name:
                    cc_value = convert_parameter_value(preset_param_name, value_type, value, PARAMETER_TABLE_MAP)
                    preset_data[your_param_name] = cc_value
                    param_details[your_param_name] = f"{value}->{cc_value}"
                    mapped_count += 1
                    mapped = True
                    break
        
        # Add the extracted keyboard ranges and trill to our preset data
        if range_data:
            print(f"\nADDING EXTRACTED RANGES TO PRESET DATA:")
            
            # Map the extracted ranges to the correct parameter names
            preset_data['leadBottomNote'] = range_data['lead_range'][0]
            preset_data['leadTopNote'] = range_data['lead_range'][1]
            preset_data['polyBottomNote'] = range_data['poly_range'][0]
            preset_data['polyTopNote'] = range_data['poly_range'][1]
            preset_data['stringsBottomNote'] = range_data['strings_range'][0]
            preset_data['stringsTopNote'] = range_data['strings_range'][1]
            preset_data['bassBottomNote'] = range_data['bass_range'][0]
            preset_data['bassTopNote'] = range_data['bass_range'][1]
            preset_data['trillValue'] = range_data['trill_interval']
            
            print(f"  Bass: {range_data['bass_range'][0]}-{range_data['bass_range'][1]} (bassBottomNote: {preset_data['bassBottomNote']}, bassTopNote: {preset_data['bassTopNote']})")
            print(f"  Strings: {range_data['strings_range'][0]}-{range_data['strings_range'][1]} (stringsBottomNote: {preset_data['stringsBottomNote']}, stringsTopNote: {preset_data['stringsTopNote']})")
            print(f"  Poly: {range_data['poly_range'][0]}-{range_data['poly_range'][1]} (polyBottomNote: {preset_data['polyBottomNote']}, polyTopNote: {preset_data['polyTopNote']})")
            print(f"  Lead: {range_data['lead_range'][0]}-{range_data['lead_range'][1]} (leadBottomNote: {preset_data['leadBottomNote']}, leadTopNote: {preset_data['leadTopNote']})")
            print(f"  Trill: {range_data['trill_interval']} (trillValue: {preset_data['trillValue']})")
        
        # Debug: Show the specific keyboard parameters
        print(f"\nKEYBOARD PARAMETER VALUES:")
        keyboard_params = [
            'leadBottomNote', 'leadTopNote', 'polyBottomNote', 'polyTopNote',
            'stringsBottomNote', 'stringsTopNote', 'bassBottomNote', 'bassTopNote', 'trillValue'
        ]
        for param in keyboard_params:
            print(f"  {param}: {preset_data[param]}")
        
        # Analyze mapping coverage
        mapped_params, unmapped_params, missing_params = analyze_mapping_coverage(parameters)
        
        # Create output file
        output_file = os.path.join(output_folder, str(index))
        
        # Inject waveform values from the extractor after zero-fill
        lead1, lead2, poly = extract_waveforms_from_file(file_path)
        preset_data['leadVCO1wave'] = lead1
        preset_data['leadVCO2wave'] = lead2
        preset_data['polyWave'] = poly
        print(f"Injected waveform bytes: lead1={lead1}, lead2={lead2}, poly={poly}")


        # Write as comma-separated values - use the original PARAM_MAP order
        values = [preset_name] + [str(preset_data[param]) for param in PARAM_MAP.keys()]
        
        with open(output_file, 'w', encoding='utf-8', newline='\n') as f:
            f.write(','.join(values) + '\n')
            f.flush()
            os.fsync(f.fileno())

        
        # Add to analysis data
        analysis_row = [index, preset_file, preset_name, mapped_count, len(parameters)]
        for param in PARAM_MAP.keys():
            analysis_row.append(preset_data[param])
        analysis_data.append(analysis_row)
        
        print(f"  -> Saved as: {output_file}")
        print(f"  -> Mapped {mapped_count}/{len(parameters)} parameters")
    
    # Write detailed analysis CSV
    with open(analysis_csv, 'w', newline='') as f:
        writer = csv.writer(f)
        # Header row - use the original PARAM_MAP order
        header = ['Index', 'Filename', 'Preset Name', 'Mapped Params', 'Total Params'] + list(PARAM_MAP.keys())
        writer.writerow(header)
        writer.writerows(analysis_data)
    
    print(f"\nSUMMARY:")
    print(f"Processed {len(preset_files)} preset files")
    print(f"Output folder: {output_folder}")
    print(f"Analysis file: {analysis_csv}")

# --- USAGE ---
if __name__ == "__main__":
    input_folder = "C:/Quadra"  # Change this to your folder path
    output_folder = "C:/Quadra/Output"  # Change this to your desired output folder
    
    process_preset_files(input_folder, output_folder)