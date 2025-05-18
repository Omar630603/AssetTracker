"use client"

import * as React from "react"
import { Check, ChevronsUpDown } from "lucide-react"
import { cn } from "@/lib/utils"
import { Button } from "@/components/ui/button"
import {
    Command,
    CommandEmpty,
    CommandGroup,
    CommandInput,
    CommandItem,
    CommandList,
} from "@/components/ui/command"
import {
    Popover,
    PopoverContent,
    PopoverTrigger,
} from "@/components/ui/popover"
import { PopoverClose } from "@radix-ui/react-popover"

export interface ComboboxItem {
    value: string
    label: string
}

interface ComboboxProps {
    items: ComboboxItem[]
    value?: string
    onSelect: (value: string) => void
    placeholder?: string
    emptyMessage?: string
    searchPlaceholder?: string
    className?: string
    disabled?: boolean
}

export function Combobox({
    items,
    value,
    onSelect,
    placeholder = "Select an option...",
    emptyMessage = "No items found.",
    searchPlaceholder = "Search...",
    className,
    disabled = false,
}: ComboboxProps) {
    const [open, setOpen] = React.useState(false)
    const [popoverWidth, setPopoverWidth] = React.useState<number>(0)
    const buttonRef = React.useRef<HTMLButtonElement>(null)

    // Find the currently selected item label to display
    const selectedItem = React.useMemo(() => {
        return items.find((item) => item.value === value)
    }, [items, value])

    // Update the popover width when the button ref changes or when open changes
    React.useEffect(() => {
        if (buttonRef.current) {
            setPopoverWidth(buttonRef.current.offsetWidth)
        }
    }, [buttonRef, open])

    return (
        <Popover open={open} onOpenChange={setOpen} modal={true}>
            <PopoverTrigger asChild>
                <Button
                    ref={buttonRef}
                    variant="outline"
                    role="combobox"
                    aria-expanded={open}
                    disabled={disabled}
                    className={cn("w-full justify-between", className)}
                >
                    {selectedItem ? selectedItem.label : placeholder}
                    <ChevronsUpDown className="ml-2 h-4 w-4 shrink-0 opacity-50" />
                </Button>
            </PopoverTrigger>
            <PopoverContent
                className="p-0"
                style={{ width: `${popoverWidth}px` }}
                align="start"
            >
                <Command className="w-full">
                    <CommandInput placeholder={searchPlaceholder} className="h-9" />
                    <CommandList>
                        <CommandEmpty>{emptyMessage}</CommandEmpty>
                        <CommandGroup>
                            <PopoverClose asChild>
                                <div>
                                    {items.map((item) => (
                                        <CommandItem
                                            key={item.value}
                                            onSelect={() => {
                                                onSelect(item.value)
                                                setOpen(false)
                                            }}
                                        >
                                            {item.label}
                                            <Check
                                                className={cn(
                                                    "ml-auto h-4 w-4",
                                                    value === item.value ? "opacity-100" : "opacity-0"
                                                )}
                                            />
                                        </CommandItem>
                                    ))}
                                </div>
                            </PopoverClose>
                        </CommandGroup>
                    </CommandList>
                </Command>
            </PopoverContent>
        </Popover>
    )
}
