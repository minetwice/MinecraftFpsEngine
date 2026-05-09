package com.minefilter.agent;

import org.objectweb.asm.*;

import java.lang.instrument.ClassFileTransformer;
import java.security.ProtectionDomain;

public class RendererTransformer implements ClassFileTransformer {
    @Override
    public byte[] transform(ClassLoader loader, String className,
                            Class<?> classBeingRedefined,
                            ProtectionDomain protectionDomain,
                            byte[] classfileBuffer) {
        if ("net/minecraft/client/renderer/GameRenderer".equals(className)) {
            ClassReader cr = new ClassReader(classfileBuffer);
            ClassWriter cw = new ClassWriter(cr, ClassWriter.COMPUTE_MAXS);
            ClassVisitor cv = new ClassVisitor(Opcodes.ASM9, cw) {
                @Override
                public MethodVisitor visitMethod(int access, String name, String desc,
                                                 String signature, String[] exceptions) {
                    MethodVisitor mv = super.visitMethod(access, name, desc, signature, exceptions);
                    if ("render".equals(name) && "(FJ)V".equals(desc)) {
                        return new MethodVisitor(Opcodes.ASM9, mv) {
                            @Override
                            public void visitCode() {
                                // Inject call to our FPS governor at method start
                                mv.visitMethodInsn(Opcodes.INVOKESTATIC,
                                    "com/minefilter/agent/FpsGovernor",
                                    "adjustQuality",
                                    "()V",
                                    false);
                                super.visitCode();
                            }
                        };
                    }
                    return mv;
                }
            };
            cr.accept(cv, 0);
            return cw.toByteArray();
        }
        return null;
    }
}
