from PIL import Image
import os

def slice_icons():
    try:
        img = Image.open('x:/Code/DogePet/companion/static/tab_icons_master.png')
        width, height = img.size
        
        # Assuming 3x3 grid layout (standard for square gen) or 3x2.
        # Let's try to detect or just assume 3 cols, 3 rows for a square image.
        cols = 3
        rows = 3
        
        cell_w = width // cols
        cell_h = height // rows
        
        icons = [
            'icon_appearance.png', 'icon_testing.png', 'icon_audio.png',
            'icon_motion.png', 'icon_power.png', 'icon_hardware.png'
        ]
        
        # Crop top 2 rows
        count = 0
        for r in range(2): # Only first 2 rows
            for c in range(3):
                if count >= len(icons): break
                
                left = c * cell_w
                upper = r * cell_h
                right = left + cell_w
                lower = upper + cell_h
                
                # Crop and resize to 64x64 for UI consistency
                cell = img.crop((left, upper, right, lower))
                cell = cell.resize((32, 32), Image.NEAREST) # Pixel art resize
                
                save_path = f'x:/Code/DogePet/companion/static/{icons[count]}'
                cell.save(save_path)
                print(f"Saved {save_path}")
                count += 1
                
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    slice_icons()
