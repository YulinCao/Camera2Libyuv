package com.cyl.camera2libyuv.camera.camera2;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.media.Image;
import android.media.ImageReader;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Range;
import android.util.Size;
import android.view.Surface;

import androidx.core.app.ActivityCompat;

import com.cyl.camera2libyuv.utils.LibyuvUtils;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

public class Camera2Activity extends Activity {

    private static String TAG = Camera2Activity.class.getName();

    long start_time;

    private static Range<Integer>[] fpsRanges;

    // camera
    private int mCameraId = CameraCharacteristics.LENS_FACING_FRONT; // 要打开的摄像头ID
        private Size mPreviewSize = new Size(640, 480); // 固定640*480演示?
//    private Size mPreviewSize = new Size(640, 480); // 固定640*480演示

//    private Size mPreviewSize = new Size(1920, 1080); // 固定?640*480演示

    private CameraDevice mCameraDevice; // 相机对象
    private CameraCaptureSession mCaptureSession;

    // handler
    private Handler mBackgroundHandler;
    private HandlerThread mBackgroundThread;

    // output
    private Surface mPreviewSurface; // 输出到屏幕的预览
    private ImageReader mImageReader; // 预览回调的接收者

    private static final int REQUEST_CAMERA = 0x01;

    @Override
    protected void onCreate( Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);


        if ( ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                == PackageManager.PERMISSION_GRANTED &&  ActivityCompat.checkSelfPermission(this,  Manifest.permission.WRITE_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED) {
            openCamera();
        } else {
            ActivityCompat.requestPermissions(this,
                    new String[]{
                            Manifest.permission.CAMERA,
                            Manifest.permission.WRITE_EXTERNAL_STORAGE
                    }, REQUEST_CAMERA);
        }

    }

    @Override
    public void onRequestPermissionsResult(int requestCode,String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch (requestCode) {
            // 相机权限
            case REQUEST_CAMERA:
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    openCamera();
                }
                break;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    private void startBackgroundThread() {
        if (mBackgroundThread == null || mBackgroundHandler == null) {
            Log.v(TAG, "startBackgroundThread");
            mBackgroundThread = new HandlerThread("CameraBackground");
            mBackgroundThread.start();
            mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
        }
    }

    private void stopBackgroundThread() {
        Log.v(TAG, "stopBackgroundThread");
        mBackgroundThread.quitSafely();
        try {
            mBackgroundThread.join();
            mBackgroundThread = null;
            mBackgroundHandler = null;
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void init(){
        String path = Environment.getExternalStorageDirectory().getPath().toString() + "/swordfish.log";

        String pathResult = Environment.getExternalStorageDirectory().getPath().toString() + "/result.log";

        Log.d(TAG,pathResult);
    }

    public void releaseCamera() {
        Log.v(TAG, "releaseCamera");
        if (mImageReader != null) {
            mImageReader.close();
            mImageReader = null;
        }
        if (mCaptureSession != null) {
//            mCaptureSession.close();
            mCaptureSession = null;
        }
        if (mCameraDevice != null) {
            mCameraDevice.close();
            mCameraDevice = null;
        }
        stopBackgroundThread(); // 对应 openCamera() 方法中的 startBackgroundThread()
    }

    private void initPreviewRequest() {
        try {
            final CaptureRequest.Builder builder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            // 添加输出到ImageReader的surface。然后我们就可以从ImageReader中获取预览数据了
            builder.addTarget(mImageReader.getSurface());
            builder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, fpsRanges[0]);
            Log.d("camera", "fps:"+ fpsRanges[0]+" length:"+fpsRanges.length);
            mCameraDevice.createCaptureSession(Arrays.asList(mImageReader.getSurface()),
                    new CameraCaptureSession.StateCallback() {

                        @Override
                        public void onConfigured( CameraCaptureSession session) {
                            mCaptureSession = session;
                            // 设置连续自动对焦和自动曝光
//                            builder.set(CaptureRequest.CONTROL_AF_MODE,
//                                    CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
//                            builder.set(CaptureRequest.CONTROL_AE_MODE,
//                                    CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH);
                            CaptureRequest captureRequest = builder.build();
                            try {
                                // 一直发送预览请求
                                mCaptureSession.setRepeatingRequest(captureRequest, null, mBackgroundHandler);
                                Log.d(TAG, "setRepeatingRequest");
                            } catch (CameraAccessException e) {
                                e.printStackTrace();
                            }
                        }

                        @Override
                        public void onConfigureFailed( CameraCaptureSession session) {
                            Log.e(TAG, "ConfigureFailed. session: mCaptureSession");
                        }
                    }, mBackgroundHandler); // handle 传入 null 表示使用当前线程的 Looper
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }
    /**
     * 打开摄像头的回调
     */
    private CameraDevice.StateCallback mStateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice camera) {
            Log.d(TAG, "onOpened");
            mCameraDevice = camera;
            initPreviewRequest();
        }

        @Override
        public void onDisconnected(CameraDevice camera) {
            Log.d(TAG, "onDisconnected");
            releaseCamera();
        }

        @Override
        public void onError( CameraDevice camera, int error) {
            Log.e(TAG, "Camera Open failed, error: " + error);
            releaseCamera();
        }
    };

    @SuppressLint("MissingPermission")
    public void openCamera() {
        init();
        Log.v(TAG, "openCamera");
        startBackgroundThread(); // 对应 releaseCamera() 方法中的 stopBackgroundThread()
        try {

            CameraManager cameraManager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
            Log.d(TAG, "preview size: " + mPreviewSize.getWidth() + "*" + mPreviewSize.getHeight());
            mImageReader = ImageReader.newInstance(mPreviewSize.getWidth(), mPreviewSize.getHeight(),
                    ImageFormat.YUV_420_888, 2);
            mImageReader.setOnImageAvailableListener(mOnImageAvailableListener, null);
            // 打开摄像头
            CameraCharacteristics characteristics = cameraManager.getCameraCharacteristics(Integer.toString(mCameraId));

            //获取相机帧数范围
            fpsRanges = characteristics.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
            Log.d("FPS", "SYNC_MAX_LATENCY_PER_FRAME_CONTROL: " + Arrays.toString(fpsRanges));

//例如：输出“SYNC_MAX_LATENCY_PER_FRAME_CONTROL: [7 ,15][]15, 30][15, 120]选择合适的范围”

            start_time = System.currentTimeMillis();
            cameraManager.openCamera(Integer.toString(mCameraId), mStateCallback, mBackgroundHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    byte[] y ;
    byte[] u ;
    byte[] v ;

    byte[] d_y ;
    byte[] d_u ;
    byte[] d_v ;


    byte[] bArr = null;
    byte[] bArr2 = null;
    byte[] bArr3 = null;

    byte[] src;
    byte[] dst;


    public static Bitmap getBitmapImageFromYUV(byte[] bArr, int i, int i2) {
        YuvImage yuvImage = new YuvImage(bArr, ImageFormat.NV21 , i, i2, null);
        ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
        yuvImage.compressToJpeg(new Rect(0, 0, i, i2), 80, byteArrayOutputStream);
        byte[] byteArray = byteArrayOutputStream.toByteArray();
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;
        try {
            byteArrayOutputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return BitmapFactory.decodeByteArray(byteArray, 0, byteArray.length, options);
    }

    Executor executor = Executors.newSingleThreadExecutor();


    private void i420Scale(Image image){
        Image.Plane[] planes = image.getPlanes();
        final int i2 = image.getWidth() * image.getHeight();

        final int width       = image.getWidth();
        final int height      = image.getHeight();
        final int y_pixelStride = planes[0].getRowStride();
        final int u_pixelStride = planes[1].getRowStride();
        final int rowStride   = planes[1].getRowStride();
        final int remaining = planes[1].getBuffer().remaining();
//
        Log.d("CameraStride","value:" +
                String.valueOf(remaining) + " " + width+" " +rowStride);
//            int u_rowStride = y_rowStride >> 1;
//
//            if(y == null)
//                y = new byte[i2];
//            if(u == null)
//                u = new byte[planes[1].getBuffer().remaining()];
//            if(v == null)
//                v = new byte[planes[2].getBuffer().remaining()];

//            planes[0].getBuffer().get(y);
//            planes[1].getBuffer().get(u);
//            planes[2].getBuffer().get(v);

        // 一定不能忘记close
//            image.close();
//
//            int i = width * height;
//
//            int remaining = planes[0].getBuffer().remaining();
//            int remaining2 = planes[1].getBuffer().remaining();
//            int remaining3 = planes[2].getBuffer().remaining();
//            pixelStride = planes[2].getPixelStride();
//            rowStride = planes[2].getRowStride();
//            if(bArr == null)
//                bArr = new byte[(i * 3) / 2];
//            if(bArr2 == null)
//                bArr2 = new byte[remaining];
//            if(bArr3 == null)
//                bArr3 = new byte[remaining3];
//
//            planes[0].getBuffer().get(bArr2);
////            planes[1].getBuffer().get(new byte[remaining2]);
//            planes[2].getBuffer().get(bArr3);


        if(src == null)
            src = new byte[ i2 * 3 / 2];

        ByteBuffer yBuffer = planes[0].getBuffer();
        int yLen = width * height;
        yBuffer.get(src, 0, yLen);
        // U通道，对应planes[1]
        // U size = width * height / 4;
        // uBuffer.remaining() = width * height / 2;
        // pixelStride = 2
        ByteBuffer uBuffer = planes[1].getBuffer();
        int pixelStride = planes[1].getPixelStride(); // pixelStride = 2
        for (int i = 0; i < uBuffer.remaining(); i+=pixelStride) {
            src[yLen++] = uBuffer.get(i);
        }
        // V通道，对应planes[2]
        // V size = width * height / 4;
        // vBuffer.remaining() = width * height / 2;
        // pixelStride = 2
        ByteBuffer vBuffer = planes[2].getBuffer();
        pixelStride = planes[2].getPixelStride(); // pixelStride = 2
        for (int i = 0; i < vBuffer.remaining(); i+=pixelStride) {
            src[yLen++] = vBuffer.get(i);
        }

        image.close();
        executor.execute(new Runnable() {
            @Override
            public void run() {

                start_time = System.currentTimeMillis();

                int size = 416 * 416;
                int d_u_stride = size >> 1 -1 ;
//
//                    if(dst == null)
//                        dst = new byte[ size * 3 / 2];

//                    if(src == null)
//                        src = new byte[ size * 3 / 2];

                if (rowStride == width) {
//
//                        System.arraycopy(y, 0, src, 0, i2);
//                        System.arraycopy(u, 0, src, i2, remaining);
//
//                        if(d_y == null)
//                            d_y = new byte[size];
//                        if(d_u == null)
//                            d_u  = new byte[d_u_stride];
//                        if(d_v == null)
//                            d_v  = new byte[d_u_stride];

                    if(dst == null)
                        dst = new byte[size*3/2];

//                    LibyuvUtils.NV12Scale(src, y_pixelStride,
//                            width, height,
//                            dst, 416,
//                            416,416);

//                        final byte[] rgba = new byte[320 * 240 * 4];
//                        NativeUtils.NV12ToRGBAByte(dst, rgba, 320, 240);

//                        LibyuvUtils.YUV420PResize(y,width, height, width,
//                                d_y,416,416, 416);
//
//                        Bitmap bitmap = getBitmapImageFromYUV(src, 320, 240);

//                        Bitmap bitmap = null;
//                        ByteArrayOutputStream baos = null;
//                        ByteBuffer buffer = null;
//                        try {
//                            //Create bitmap
//                            bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
//                            buffer = ByteBuffer.wrap(rgba);
//                            bitmap.copyPixelsFromBuffer(buffer);//Convert YUV420 to byte array
//
//                            //Compress RGBA to JPEG/PNG/WEBP
//                            baos = new ByteArrayOutputStream();
//                            bitmap.compress(Bitmap.CompressFormat.PNG, 100, baos);
//
//                            File file = new File(Environment.getExternalStorageDirectory()+"/"+
//                                    System.currentTimeMillis()+".png");
//                            FileOutputStream outputStream = new FileOutputStream(file);
////                        byte [] bts = new byte[1024];
//                            if(bitmap.compress(Bitmap.CompressFormat.JPEG,80,outputStream))
//                                outputStream.close();
//                        } catch (Exception e) {
//                            e.printStackTrace();
//                        } finally {
//                            try {
//                                if (baos != null) {
//                                    baos.close();
//                                }
//
//                                if (buffer != null) {
//                                    buffer.clear();
//                                }
//
//                                if (bitmap != null && !bitmap.isRecycled()) {
//                                    bitmap.recycle();
//                                }
//                            } catch (IOException e) {
//                                e.printStackTrace();
//                            }
//                        }
                }

                long end_time = System.currentTimeMillis();
                Log.d("CameraTime2","time:" + String.valueOf(end_time - start_time));

                start_time = end_time;

            }
        });

    }

    private void nv21Scale(Image image){
        Image.Plane[] planes = image.getPlanes();

        final int i2 = image.getWidth() * image.getHeight();

        final int width       = image.getWidth();
        final int height      = image.getHeight();
        final int y_pixelStride = planes[0].getRowStride();
        final int u_pixelStride = planes[1].getRowStride();
        final int rowStride   = planes[1].getRowStride();
        final int remaining = planes[1].getBuffer().remaining();

        Log.d("CameraStride","value:" +
                String.valueOf(remaining) + " " + width+" " +rowStride);

        start_time = System.currentTimeMillis();
        if(y == null)
            y = new byte[i2];
        if(v == null)
            v = new byte[i2/2 -1];

        planes[0].getBuffer().get(y);
        planes[2].getBuffer().get(v);

        // 一定不能忘记close
        image.close();

        executor.execute(new Runnable() {
            @Override
            public void run() {

                int size = 416 * 416;

                if (rowStride == width) {

                    if(src == null)
                        src = new byte[i2/2];

                    System.arraycopy(v, 0, src, 0, i2/2 -1);

                    if(d_y == null)
                        d_y = new byte[size];
                    if(d_v == null)
                        d_v = new byte[size/2];

                    LibyuvUtils.NV21Scale(y, y_pixelStride, src, y_pixelStride,
                            width, height,
                            d_y, 416,d_v,416,
                            416,416);

                    if(dst == null)
                        dst = new byte[size *3/2];

                    System.arraycopy(d_y, 0, dst, 0, size);
                    System.arraycopy(d_v, 0, dst, size, size/2 -1);

                    Bitmap bitmap = getBitmapImageFromYUV(dst, 416, 416);

                    try {
                        File file = new File(Environment.getExternalStorageDirectory()+"/"+
                                System.currentTimeMillis()+".png");
                        FileOutputStream outputStream = new FileOutputStream(file);
//                        byte [] bts = new byte[1024];
                        if(bitmap.compress(Bitmap.CompressFormat.PNG,100,outputStream))
                            outputStream.close();
                    } catch (Exception e) {
                        e.printStackTrace();
                    } finally {
                        if (bitmap != null && !bitmap.isRecycled()) {
                            bitmap.recycle();
                        }
                    }
                }

                long end_time = System.currentTimeMillis();
                Log.d("CameraTime2","time:" + String.valueOf(end_time - start_time));

                start_time = end_time;

            }
        });

    }

    private void nv12Scale(Image image){
        Image.Plane[] planes = image.getPlanes();
        final int i2         = image.getWidth() * image.getHeight();

        final int width         = image.getWidth();
        final int height        = image.getHeight();
        final int y_pixelStride = planes[0].getRowStride();
        final int rowStride     = planes[1].getRowStride();
        final int remaining     = planes[1].getBuffer().remaining();

        Log.d("CameraStride","value:" +
                String.valueOf(remaining) + " " + width+" " +rowStride);

        start_time = System.currentTimeMillis();
        if(y == null)
            y = new byte[i2];
        if(u == null)
            u = new byte[i2/2 -1];

        planes[0].getBuffer().get(y);
        planes[1].getBuffer().get(u);

        // 一定不能忘记close
        image.close();

        executor.execute(new Runnable() {
            @Override
            public void run() {

                int size = 416 * 416;

                if (rowStride == width) {

                    if(src == null)
                        src = new byte[i2/2];
                    System.arraycopy(u, 0, src, 0, i2/2 -1);

                    if(d_y == null)
                        d_y = new byte[size];
                    if(d_u == null)
                        d_u = new byte[size/2];
                    LibyuvUtils.NV12Scale(y, y_pixelStride, src, y_pixelStride,
                            width, height,
                            d_y, 416, d_u,416,
                            416,416);

                    if(dst == null)
                        dst = new byte[size *3/2];
                    System.arraycopy(d_y, 0, dst, 0, size);
                    System.arraycopy(d_u, 0, dst, size, size/2 -1);

                }

                long end_time = System.currentTimeMillis();
                Log.d("CameraTime2","time:" + String.valueOf(end_time - start_time));
                start_time = end_time;

            }
        });

    }

    private ImageReader.OnImageAvailableListener mOnImageAvailableListener
            = new ImageReader.OnImageAvailableListener() {

        @Override
        public void onImageAvailable(ImageReader reader) {

            final Image image = reader.acquireNextImage();
            if (image == null) return;

            nv12Scale(image);
//            nv21Scale(image);

        }
    };
}
