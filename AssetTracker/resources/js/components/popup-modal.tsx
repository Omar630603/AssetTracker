"use client";

import React from 'react';
import {
    Dialog,
    DialogContent,
    DialogHeader,
    DialogTitle,
    DialogDescription,
    DialogFooter,
    DialogClose
} from '@/components/ui/dialog';
import { cn } from '@/lib/utils';
import { Button } from '@/components/ui/button';

export type ModalVariant = 'view' | 'confirm';

interface ModalProps {
    isOpen: boolean;
    onClose: () => void;
    title: string;
    description?: string;
    children?: React.ReactNode;
    variant?: ModalVariant;
    primaryActionLabel?: string;
    secondaryActionLabel?: string;
    onPrimaryAction?: () => void;
    onSecondaryAction?: () => void;
    size?: 'sm' | 'md' | 'lg' | 'xl' | 'full';
    className?: string;
    footerContent?: React.ReactNode;
    isLoading?: boolean;
    disablePrimaryAction?: boolean;
}

export function PopupModal({
    isOpen,
    onClose,
    title,
    description,
    children,
    variant = 'view',
    primaryActionLabel,
    secondaryActionLabel,
    onPrimaryAction,
    onSecondaryAction,
    size = 'md',
    className,
    footerContent,
    isLoading = false,
    disablePrimaryAction = false,
}: ModalProps) {
    const sizeClasses = {
        sm: 'sm:max-w-[400px]',
        md: 'sm:max-w-[600px]',
        lg: 'sm:max-w-[800px]',
        xl: 'sm:max-w-[1000px]',
        full: 'sm:max-w-[90vw] sm:h-[80vh]',
    };

    return (
        <Dialog open={isOpen} onOpenChange={(open) => !open && onClose()}>
            <DialogContent className={cn(sizeClasses[size], className)}>
                <DialogHeader>
                    <DialogTitle>{title}</DialogTitle>
                    {description && <DialogDescription>{description}</DialogDescription>}
                </DialogHeader>

                <div className="py-2">{children}</div>

                {variant !== 'view' &&
                    <DialogFooter>
                        {footerContent && <div className="mr-auto">{footerContent}</div>}

                        {secondaryActionLabel && (
                            <DialogClose asChild>
                                <Button variant="outline" onClick={onSecondaryAction || onClose}>
                                    {secondaryActionLabel}
                                </Button>
                            </DialogClose>
                        )}
                        <Button
                            variant="default"
                            onClick={onPrimaryAction || onClose}
                            disabled={isLoading || disablePrimaryAction}
                        >
                            {isLoading ? 'Loading...' : primaryActionLabel || 'Confirm'}
                        </Button>
                    </DialogFooter>
                }
            </DialogContent>
        </Dialog>
    );
}
