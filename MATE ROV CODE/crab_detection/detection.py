import cv2
import numpy as np

# ── Config ──────────────────────────────────────────────────────────────────
MIN_GOOD_MATCHES   = 12    # raise if you get false positives
LOWE_RATIO         = 0.75  # standard Lowe's ratio test threshold
MIN_INLIERS        = 8     # minimum RANSAC inliers to confirm a detection
BOX_COLORS = {             # BGR colors per crab type
    "euro":  (0,   255, 0),
    "jonah": (0,   165, 255),
    "rock":  (255, 0,   0),
}

# ── Load reference images ────────────────────────────────────────────────────
refs = {
    "euro":  cv2.imread("image_refs/euro_crab.jpg"),
    "jonah": cv2.imread("image_refs/jonah_crab.png"),
    "rock":  cv2.imread("image_refs/rock_crab.jpg"),
}

refs_gray = {}
for name, img in refs.items():
    if img is None:
        print(f"[WARN] Could not load reference image for '{name}' — skipping.")
    else:
        refs_gray[name] = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

# ── Compute ORB keypoints & descriptors for every reference ──────────────────
orb = cv2.ORB_create(nfeatures=1500)
ref_data = {}
for name, gray in refs_gray.items():
    kp, des = orb.detectAndCompute(gray, None)
    if des is None or len(kp) == 0:
        print(f"[WARN] No features found for '{name}' — skipping.")
        continue
    h, w = gray.shape
    # Corner points of the reference image (used to project bounding box)
    corners = np.float32([[0, 0], [w, 0], [w, h], [0, h]]).reshape(-1, 1, 2)
    ref_data[name] = {"kp": kp, "des": des, "corners": corners}

if not ref_data:
    raise RuntimeError("No reference images loaded successfully. Check image_refs/ folder.")

bf  = cv2.BFMatcher(cv2.NORM_HAMMING)
cap = cv2.VideoCapture(1) #instead of 0, cycle through 1 or 2 to hit the usb signal. internal cam = usb input. 

print("Running — press  Q  to quit.")

while True:
    success, frame = cap.read()
    if not success:
        break

    frame_gray          = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    frame_kp, frame_des = orb.detectAndCompute(frame_gray, None)

    counts = {name: 0 for name in ref_data}  # detections per crab type
    
    print("Running — press  Q  to quit. 1")

    if frame_des is not None and len(frame_kp) > 0:
        for name, data in ref_data.items():
            color = BOX_COLORS.get(name, (255, 255, 255))

            # ── Lowe's ratio test ────────────────────────────────────────────
            raw_matches = bf.knnMatch(data["des"], frame_des, k=2)
            good = [m for pair in raw_matches
                    if len(pair) == 2
                    for m, n in [pair]
                    if m.distance < LOWE_RATIO * n.distance]

            if len(good) < MIN_GOOD_MATCHES:
                continue  # not enough evidence — skip

            # ── Homography (RANSAC) to find exact location ───────────────────
            src_pts = np.float32(
                [data["kp"][m.queryIdx].pt for m in good]
            ).reshape(-1, 1, 2)
            dst_pts = np.float32(
                [frame_kp[m.trainIdx].pt for m in good]
            ).reshape(-1, 1, 2)

            H, mask = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC, 5.0)

            if H is None:
                continue

            inliers = int(mask.sum()) if mask is not None else 0
            if inliers < MIN_INLIERS:
                continue  # RANSAC didn't find a consistent transformation

            # ── Project reference corners into the frame ─────────────────────
            projected = cv2.perspectiveTransform(data["corners"], H)

            # Convert to axis-aligned bounding rect
            x, y, w, h = cv2.boundingRect(projected.reshape(-1, 2).astype(np.int32))

            # Basic sanity check — ignore absurdly large or tiny boxes
            fh, fw = frame.shape[:2]
            if w < 20 or h < 20 or w > fw * 0.95 or h > fh * 0.95:
                continue

            counts[name] += 1

            # ── Draw the bounding box and label ──────────────────────────────
            cv2.rectangle(frame, (x, y), (x + w, y + h), color, 2)
            label = f"{name} crab"
            (tw, th), _ = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.7, 2)
            cv2.rectangle(frame, (x, y - th - 8), (x + tw + 4, y), color, -1)
            cv2.putText(frame, label,
                        (x + 2, y - 4),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

    # ── HUD — crab counts in the corner ─────────────────────────────────────
    y_offset = 30
    for i, (name, count) in enumerate(counts.items()):
        color  = BOX_COLORS.get(name, (255, 255, 255))
        status = str(count) if count > 0 else "—"
        text   = f"{name}: {status}"
        cv2.putText(frame, text,
                    (15, y_offset + i * 32),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.85, color, 2)

    cv2.imshow("Crab Detector", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()